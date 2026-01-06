import { useState, useCallback, useRef } from 'react';
import { Capacitor } from '@capacitor/core';
import { BluetoothLe } from '@capacitor-community/bluetooth-le';

export const useBLE = (serviceUuid: string, characteristicUuid: string) => {
  const [device, setDevice] = useState<any | null>(null);
  const [isConnected, setIsConnected] = useState(false);
  const [isConnecting, setIsConnecting] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [devices, setDevices] = useState<any[]>([]);
  const [connectedAt, setConnectedAt] = useState<number | null>(null);
  const writingRef = useRef(false);
  const lastWriteTsRef = useRef(0);
  const supportsWriteNoRespRef = useRef<boolean | null>(null);
  const queuedLatestRef = useRef<string | null>(null);
  const queueTimerRef = useRef<any>(null);
  const processingRef = useRef(false);

  const directionCharacteristic = useRef<any | null>(null);
  const nativeDeviceIdRef = useRef<string | null>(null);

  const base64EncodeAscii = (str: string) => {
    let bin = '';
    for (let i = 0; i < str.length; i++) {
      const code = str.charCodeAt(i);
      bin += String.fromCharCode(code & 0xff);
    }
    // @ts-ignore
    if (typeof btoa !== 'undefined') return btoa(bin);
    const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=';
    let output = '';
    let i = 0;
    while (i < bin.length) {
      const a = bin.charCodeAt(i++) || 0;
      const b = bin.charCodeAt(i++) || 0;
      const c = bin.charCodeAt(i++) || 0;
      const enc1 = a >> 2;
      const enc2 = ((a & 3) << 4) | (b >> 4);
      const enc3 = ((b & 15) << 2) | (c >> 6);
      const enc4 = c & 63;
      if (!b) {
        output += chars.charAt(enc1) + chars.charAt(enc2) + '==';
      } else if (!c) {
        output += chars.charAt(enc1) + chars.charAt(enc2) + chars.charAt(enc3) + '=';
      } else {
        output += chars.charAt(enc1) + chars.charAt(enc2) + chars.charAt(enc3) + chars.charAt(enc4);
      }
    }
    return output;
  };

  const handleDisconnected = useCallback(() => {
    setIsConnected(false);
    setDevice(null);
    directionCharacteristic.current = null;
    nativeDeviceIdRef.current = null;
  }, []);

  const ensureEnabled = useCallback(async () => {
    if (Capacitor.isNativePlatform()) {
      const enabled = await BluetoothLe.isEnabled();
      if (!enabled.value) {
        await BluetoothLe.enable();
      }
    }
  }, []);

  const scanForDevices = useCallback(async () => {
    setError(null);
    setIsConnecting(true);
    try {
      if (Capacitor.isNativePlatform()) {
        await BluetoothLe.initialize({ androidNeverForLocation: true });
        try { await BluetoothLe.requestPermissions(); } catch {}
        const btEnabled = await BluetoothLe.isEnabled();
        if (!btEnabled.value) { await BluetoothLe.enable(); }
        const locEnabled = await BluetoothLe.isLocationEnabled();
        if (!locEnabled.value) {
          setError('Location is OFF. Please enable location services to scan BLE on Android.');
          await BluetoothLe.openLocationSettings();
          setIsConnecting(false);
          return;
        }
        try {
          const picked = await BluetoothLe.requestDevice({ services: [serviceUuid] });
          if (picked && picked.deviceId) {
            setDevices([picked]);
            setIsConnecting(false);
            return;
          }
        } catch {}
        setDevices([]);
        await new Promise<void>(async (resolve) => {
          await BluetoothLe.requestLEScan({ allowDuplicates: false, scanMode: 2 }, (result:any) => {
            if (result && result.device && result.device.deviceId) {
              setDevices(prev => {
                const exists = prev.some(x => x.deviceId === result.device.deviceId);
                if (exists) return prev;
                return [...prev, result.device];
              });
            }
          });
          setTimeout(async () => {
            await BluetoothLe.stopLEScan();
            resolve();
          }, 10000);
        });
        await BluetoothLe.stopLEScan();
        if (devices.length === 0) {
          try {
            const picked = await BluetoothLe.requestDevice({ services: [serviceUuid] });
            if (picked && picked.deviceId) {
              setDevices([picked]);
            }
          } catch {}
        }
      } else {
        const selectedDevice = await (navigator as any).bluetooth.requestDevice({ filters: [{ services: [serviceUuid] }] });
        setDevice(selectedDevice);
        selectedDevice.addEventListener('gattserverdisconnected', handleDisconnected);
        const server = await selectedDevice.gatt?.connect();
        if (!server) throw new Error('Could not connect to GATT server.');
        const service = await server.getPrimaryService(serviceUuid);
        const characteristic = await service.getCharacteristic(characteristicUuid);
        directionCharacteristic.current = characteristic;
        setIsConnected(true);
      }
    } catch (err:any) {
      setError(err.message || 'Failed to connect to the BLE device.');
      setIsConnected(false);
      setDevice(null);
    } finally {
      setIsConnecting(false);
    }
  }, [serviceUuid, characteristicUuid, handleDisconnected]);

  const connectToDevice = useCallback(async (deviceId: string) => {
    setError(null);
    setIsConnecting(true);
    try {
      if (Capacitor.isNativePlatform()) {
        await BluetoothLe.initialize({ androidNeverForLocation: true });
        try { await BluetoothLe.requestPermissions(); } catch {}
        await ensureEnabled();
        await BluetoothLe.connect({ deviceId });
        try { await BluetoothLe.requestConnectionPriority({ deviceId, connectionPriority: 2 }); } catch {}
        try { await BluetoothLe.discoverServices({ deviceId }); } catch {}
        try { const _s = await BluetoothLe.getServices({ deviceId }); supportsWriteNoRespRef.current = true; } catch {}
        try { await BluetoothLe.requestMtu({ deviceId, mtu: 185 }); } catch {}
        try { await BluetoothLe.getMtu({ deviceId }); } catch {}
        nativeDeviceIdRef.current = deviceId;
        const found = devices.find(d => d.deviceId === deviceId) || { deviceId };
        setDevice(found);
        setIsConnected(true);
        setConnectedAt(Date.now());
      }
    } catch (err:any) {
      setError(err.message || 'Failed to connect to the BLE device.');
      setIsConnected(false);
      setDevice(null);
    } finally {
      setIsConnecting(false);
    }
  }, [devices, ensureEnabled]);

  const disconnect = useCallback(async () => {
    try {
      if (Capacitor.isNativePlatform()) {
        if (nativeDeviceIdRef.current) {
          await BluetoothLe.disconnect({ deviceId: nativeDeviceIdRef.current });
        }
      } else {
        if (device && device.gatt?.connected) {
          device.gatt.disconnect();
        }
      }
    } finally {
      handleDisconnected();
      setConnectedAt(null);
      supportsWriteNoRespRef.current = null;
      queuedLatestRef.current = null;
      if (queueTimerRef.current) { clearTimeout(queueTimerRef.current); queueTimerRef.current = null; }
    }
  }, [device, handleDisconnected]);

  const processQueue = useCallback(async () => {
    if (writingRef.current) return;
    const latest = queuedLatestRef.current;
    if (!latest || !isConnected) return;
    queuedLatestRef.current = null;
    writingRef.current = true;
    try {
      if (Capacitor.isNativePlatform()) {
        if (!nativeDeviceIdRef.current) return;
        try {
          const con = await BluetoothLe.isConnected({ deviceId: nativeDeviceIdRef.current });
          if (!con.value) {
            await BluetoothLe.connect({ deviceId: nativeDeviceIdRef.current });
            await new Promise(r => setTimeout(r, 100));
          }
        } catch {}
        const safePayload = latest.replace(/[^A-Za-z0-9]/g, '');
        const isValid = /^[A-Za-z][0-9]{0,2}$/.test(safePayload) || /^[A-Za-z]$/.test(safePayload);
        if (!isValid) return;
        const now = Date.now();
        if (now - lastWriteTsRef.current < 1000) {
          queuedLatestRef.current = latest;
          if (!queueTimerRef.current) {
            const delay = Math.max(1000 - (now - lastWriteTsRef.current), 50);
            queueTimerRef.current = setTimeout(() => {
              queueTimerRef.current = null;
              processQueue();
            }, delay);
          }
          return;
        }
        const withTerminator = safePayload + "\n";
        const payload = withTerminator.length > 20 ? withTerminator.slice(0, 20) : withTerminator;
        const value = base64EncodeAscii(payload);
        const svc = (serviceUuid || '').toLowerCase();
        const chr = (characteristicUuid || '').toLowerCase();
        try { console.log('BLE write payload', { payload, svc, chr, deviceId: nativeDeviceIdRef.current }); } catch {}
        await new Promise(r => setTimeout(r, 150));
        try {
          await BluetoothLe.write({ deviceId: nativeDeviceIdRef.current, service: svc, characteristic: chr, value });
          try { console.log('BLE write success'); } catch {}
        } catch (e:any) {
          try {
            await BluetoothLe.writeWithoutResponse({ deviceId: nativeDeviceIdRef.current, service: svc, characteristic: chr, value });
            try { console.log('BLE writeWithoutResponse success'); } catch {}
          } catch (e2:any) {
            setError(e2?.message || e?.message || 'Failed to send direction.');
            return;
          }
        }
      } else {
        if (!directionCharacteristic.current) return;
        const encoder = new TextEncoder();
        const safePayloadWeb = latest.replace(/[^A-Za-z0-9]/g, '');
        const isValidWeb = /^[A-Za-z][0-9]{0,2}$/.test(safePayloadWeb) || /^[A-Za-z]$/.test(safePayloadWeb);
        if (!isValidWeb) return;
        const value = encoder.encode(safePayloadWeb + "\n");
        if ((directionCharacteristic.current as any)?.writeValueWithoutResponse) {
          await (directionCharacteristic.current as any).writeValueWithoutResponse(value);
        } else {
          await directionCharacteristic.current.writeValue(value);
        }
      }
      lastWriteTsRef.current = Date.now();
    } catch (err:any) {
      setError(err.message || 'Failed to send direction.');
    } finally {
      writingRef.current = false;
      if (queuedLatestRef.current) {
        processQueue();
      }
    }
  }, [isConnected, serviceUuid, characteristicUuid]);

  const writeDirection = useCallback(async (text: string) => {
    queuedLatestRef.current = text;
    if (!queueTimerRef.current) {
      queueTimerRef.current = setTimeout(() => {
        queueTimerRef.current = null;
        processQueue();
      }, 1000);
    }
  }, [processQueue]);

  return { device, devices, isConnected, isConnecting, error, connectedAt, connect: scanForDevices, connectToDevice, disconnect, writeDirection };
};

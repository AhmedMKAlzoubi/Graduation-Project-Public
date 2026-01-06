package com.smart.helmet;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.Service;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattService;
import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.os.IBinder;
import android.util.Log;

import java.nio.charset.StandardCharsets;
import java.util.LinkedList;
import java.util.Queue;
import java.util.UUID;

public class BleForegroundService extends Service {
    public static final String ACTION_START = "com.smart.helmet.ble.START";
    public static final String ACTION_SEND = "com.smart.helmet.ble.SEND";
    public static final String ACTION_STOP = "com.smart.helmet.ble.STOP";

    private static final String CHANNEL_ID = "ble_service_channel";
    private static final int NOTIFICATION_ID = 1001;

    private String deviceId;
    private UUID serviceUuid;
    private UUID characteristicUuid;

    private BluetoothAdapter adapter;
    private BluetoothGatt gatt;
    private BluetoothGattCharacteristic writeChar;

    private final Queue<byte[]> queue = new LinkedList<>();
    private boolean writing = false;
    private long lastWriteTs = 0L;

    @Override
    public void onCreate() {
        super.onCreate();
        adapter = BluetoothAdapter.getDefaultAdapter();
        ensureChannel();
        try {
            Notification notification = new Notification.Builder(this, CHANNEL_ID)
                    .setContentTitle("Smart Helmet BLE")
                    .setContentText("Active BLE connection")
                    .setSmallIcon(android.R.drawable.stat_sys_data_bluetooth)
                    .build();
            startForeground(NOTIFICATION_ID, notification);
        } catch (Exception e) {
            Log.w("BleService", "startForeground failed (notification permission?)", e);
        }
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        if (intent == null) return START_STICKY;
        String action = intent.getAction();
        if (ACTION_START.equals(action)) {
            deviceId = intent.getStringExtra("deviceId");
            String svc = intent.getStringExtra("serviceUuid");
            String chr = intent.getStringExtra("characteristicUuid");
            try {
                serviceUuid = UUID.fromString(svc);
                characteristicUuid = UUID.fromString(chr);
            } catch (Exception e) {
                Log.e("BleService", "Invalid UUIDs", e);
            }
            connectGatt();
        } else if (ACTION_SEND.equals(action)) {
            String payload = intent.getStringExtra("payload");
            if (payload != null) {
                enqueue(toBytes(payload + "\n"));
            }
        } else if (ACTION_STOP.equals(action)) {
            disconnect();
            stopForeground(true);
            stopSelf();
        }
        return START_STICKY;
    }

    private void ensureChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            NotificationChannel channel = new NotificationChannel(CHANNEL_ID, "BLE Service", NotificationManager.IMPORTANCE_LOW);
            NotificationManager nm = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
            nm.createNotificationChannel(channel);
        }
    }

    private void connectGatt() {
        if (adapter == null || deviceId == null) return;
        try {
            BluetoothDevice device = adapter.getRemoteDevice(deviceId);
            gatt = device.connectGatt(this, false, callback, BluetoothDevice.TRANSPORT_LE);
        } catch (Exception e) {
            Log.e("BleService", "connectGatt error", e);
        }
    }

    private final BluetoothGattCallback callback = new BluetoothGattCallback() {
        @Override
        public void onConnectionStateChange(BluetoothGatt g, int status, int newState) {
            if (newState == android.bluetooth.BluetoothProfile.STATE_CONNECTED) {
                g.discoverServices();
            } else if (newState == android.bluetooth.BluetoothProfile.STATE_DISCONNECTED) {
                writeChar = null;
            }
        }

        @Override
        public void onServicesDiscovered(BluetoothGatt g, int status) {
            BluetoothGattService svc = g.getService(serviceUuid);
            if (svc != null) {
                writeChar = svc.getCharacteristic(characteristicUuid);
            }
            drain();
        }

        @Override
        public void onCharacteristicWrite(BluetoothGatt g, BluetoothGattCharacteristic characteristic, int status) {
            writing = false;
            lastWriteTs = System.currentTimeMillis();
            drain();
        }
    };

    private void enqueue(byte[] data) {
        if (data == null || data.length == 0) return;
        queue.offer(data);
        drain();
    }

    private void drain() {
        if (writing) return;
        if (writeChar == null || gatt == null) return;
        byte[] next = queue.peek();
        if (next == null) return;
        long now = System.currentTimeMillis();
        if (now - lastWriteTs < 1000) return;
        try {
            int props = writeChar.getProperties();
            boolean noResp = (props & BluetoothGattCharacteristic.PROPERTY_WRITE_NO_RESPONSE) != 0;
            boolean withResp = (props & BluetoothGattCharacteristic.PROPERTY_WRITE) != 0;
            if (noResp) {
                writeChar.setWriteType(BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE);
            } else {
                writeChar.setWriteType(BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT);
            }
            writeChar.setValue(next);
            boolean ok = gatt.writeCharacteristic(writeChar);
            if (ok) {
                queue.poll();
                writing = true;
                if (noResp && !withResp) {
                    writing = false;
                    lastWriteTs = System.currentTimeMillis();
                    drain();
                }
            } else {
                Log.w("BleService", "writeCharacteristic returned false");
                writing = false;
            }
        } catch (Exception e) {
            Log.e("BleService", "write error", e);
            writing = false;
        }
    }

    private byte[] toBytes(String s) {
        return s.getBytes(StandardCharsets.US_ASCII);
    }

    private void disconnect() {
        try {
            if (gatt != null) {
                gatt.disconnect();
                gatt.close();
                gatt = null;
            }
            writeChar = null;
        } catch (Exception ignored) {}
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }
}


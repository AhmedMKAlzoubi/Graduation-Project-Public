import { registerPlugin } from '@capacitor/core';

export interface BleServicePlugin {
  start(options: { deviceId: string; serviceUuid: string; characteristicUuid: string }): Promise<void>;
  send(options: { payload: string }): Promise<void>;
  stop(): Promise<void>;
}

export const BleService = registerPlugin<BleServicePlugin>('BleService');


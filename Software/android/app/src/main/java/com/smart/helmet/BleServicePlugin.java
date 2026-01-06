package com.smart.helmet;

import android.content.Context;
import android.content.Intent;

import com.getcapacitor.Plugin;
import com.getcapacitor.PluginCall;
import com.getcapacitor.annotation.CapacitorPlugin;
import com.getcapacitor.PluginMethod;

@CapacitorPlugin(name = "BleService")
public class BleServicePlugin extends Plugin {
    @PluginMethod
    public void start(PluginCall call) {
        String deviceId = call.getString("deviceId");
        String serviceUuid = call.getString("serviceUuid");
        String characteristicUuid = call.getString("characteristicUuid");
        Intent intent = new Intent(getContext(), BleForegroundService.class);
        intent.setAction(BleForegroundService.ACTION_START);
        intent.putExtra("deviceId", deviceId);
        intent.putExtra("serviceUuid", serviceUuid);
        intent.putExtra("characteristicUuid", characteristicUuid);
        Context ctx = getContext();
        try { ctx.startForegroundService(intent); } catch (Exception e) { ctx.startService(intent); }
        call.resolve();
    }

    @PluginMethod
    public void send(PluginCall call) {
        String payload = call.getString("payload");
        Intent intent = new Intent(getContext(), BleForegroundService.class);
        intent.setAction(BleForegroundService.ACTION_SEND);
        intent.putExtra("payload", payload);
        getContext().startService(intent);
        call.resolve();
    }

    @PluginMethod
    public void stop(PluginCall call) {
        Intent intent = new Intent(getContext(), BleForegroundService.class);
        intent.setAction(BleForegroundService.ACTION_STOP);
        getContext().startService(intent);
        call.resolve();
    }
}

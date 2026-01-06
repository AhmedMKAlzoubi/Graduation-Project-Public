package com.smart.helmet;

import android.os.Bundle;
import com.getcapacitor.BridgeActivity;

public class MainActivity extends BridgeActivity {
  @Override
  public void onCreate(Bundle savedInstanceState) {
    registerPlugin(BleServicePlugin.class);
    super.onCreate(savedInstanceState);
  }
}

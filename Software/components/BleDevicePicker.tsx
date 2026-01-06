import React from 'react';

interface DeviceItem {
  deviceId: string;
  name?: string;
}

interface Props {
  devices: DeviceItem[];
  onConnect: (deviceId: string) => void;
  onClose: () => void;
}

const BleDevicePicker: React.FC<Props> = ({ devices, onConnect, onClose }) => {
  return (
    <div className="fixed inset-0 bg-black/60 flex items-center justify-center z-50">
      <div className="bg-gray-800 w-full max-w-md p-6 rounded-lg shadow-2xl">
        <h3 className="text-xl font-bold mb-4 text-gray-200">Select Helmet Device</h3>
        {devices.length === 0 ? (
          <p className="text-gray-400">Scanning… Make sure the helmet is powered and nearby.</p>
        ) : (
          <ul className="space-y-2">
            {devices.map((d) => (
              <li key={d.deviceId}>
                <button
                  className="w-full text-left p-3 rounded-md bg-gray-700 hover:bg-gray-700/80 text-white"
                  onClick={() => onConnect(d.deviceId)}
                >
                  <div className="font-semibold">{d.name || 'Unknown Device'}</div>
                  <div className="text-xs text-gray-300">{d.deviceId}</div>
                </button>
              </li>
            ))}
          </ul>
        )}
        <div className="mt-4 flex justify-end">
          <button className="px-4 py-2 rounded-md bg-gray-600 text-white" onClick={onClose}>Close</button>
        </div>
      </div>
    </div>
  );
};

export default BleDevicePicker;

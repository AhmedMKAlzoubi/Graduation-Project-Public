
import React from 'react';
import { Bluetooth, BluetoothConnected, Loader } from './Icons';

interface BleConnectButtonProps {
  isConnected: boolean;
  isConnecting: boolean;
  onConnect: () => void;
  onDisconnect: () => void;
}

const BleConnectButton: React.FC<BleConnectButtonProps> = ({
  isConnected,
  isConnecting,
  onConnect,
  onDisconnect,
}) => {
  const handleClick = () => {
    if (isConnected) {
      onDisconnect();
    } else {
      onConnect();
    }
  };

  const buttonText = isConnected ? 'Disconnect Helmet' : 'Connect Helmet';
  const buttonClass = isConnected
    ? 'bg-red-600 hover:bg-red-700'
    : 'bg-blue-600 hover:bg-blue-700';

  return (
    <button
      onClick={handleClick}
      disabled={isConnecting}
      className={`flex items-center justify-center gap-2 px-4 py-2 font-semibold text-white rounded-lg transition-colors duration-200 disabled:opacity-50 disabled:cursor-wait ${buttonClass}`}
    >
      {isConnecting ? (
        <>
         <Loader className="animate-spin w-5 h-5"/>
         <span>Connecting...</span>
        </>
      ) : isConnected ? (
        <>
          <BluetoothConnected className="w-5 h-5"/>
          <span>{buttonText}</span>
        </>
      ) : (
        <>
          <Bluetooth className="w-5 h-5"/>
          <span>{buttonText}</span>
        </>
      )}
    </button>
  );
};

export default BleConnectButton;

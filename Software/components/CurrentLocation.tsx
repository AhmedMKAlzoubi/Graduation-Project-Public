import React, { useState, useEffect, useRef } from 'react';
import type { GeoPosition } from '../types';
import { getAddressFromCoordinates } from '../services/navigationService';
import { MapPin, Loader } from './Icons';

interface CurrentLocationProps {
  position: GeoPosition | null;
}

const CurrentLocation: React.FC<CurrentLocationProps> = ({ position }) => {
  const [address, setAddress] = useState<string | null>(null);
  const [isLoading, setIsLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const lastUpdateRef = useRef(0);

  useEffect(() => {
    if (position) {
      const now = Date.now();
      // Throttle reverse geocoding to prevent API spam and UI flicker.
      // We'll only update the address if it's the first time or 10s have passed.
      if (now - lastUpdateRef.current < 10000 && address) {
        return;
      }
      lastUpdateRef.current = now;

      // Only show the loading spinner on the initial fetch.
      if (!address) {
        setIsLoading(true);
      }
      setError(null);
      const { latitude, longitude } = position.coords;

      getAddressFromCoordinates({ lat: latitude, lng: longitude })
        .then(formattedAddress => {
          setAddress(formattedAddress);
        })
        .catch(err => {
          console.error("Reverse geocoding error:", err);
          if (!address) { // Only set error if we don't already have an address.
            setError("Could not determine address.");
            setAddress(`${latitude.toFixed(4)}, ${longitude.toFixed(4)}`); // Fallback to coords
          }
        })
        .finally(() => {
          setIsLoading(false);
        });
    } else {
        setIsLoading(true);
        setAddress(null);
        setError(null);
    }
  }, [position, address]);

  const renderContent = () => {
    if (isLoading) {
        return (
            <div className="flex items-center gap-2 text-gray-400">
                <Loader className="animate-spin w-5 h-5" />
                <span>Determining location...</span>
            </div>
        );
    }

    if (error && !address) {
        return <span className="text-red-400">{error}</span>;
    }

    return (
        <div>
            <p className="text-cyan-400 font-medium text-center md:text-left">{address}</p>
        </div>
    );
  };
  
  return (
    <div className="bg-gray-800 p-4 rounded-lg shadow-2xl flex flex-col md:flex-row items-center gap-4">
      <div className="flex-shrink-0">
        <MapPin className="w-8 h-8 text-cyan-500" />
      </div>
      <div className="flex-grow w-full">
        <h3 className="text-lg font-bold text-white text-center md:text-left mb-1">Current Location</h3>
        {renderContent()}
      </div>
    </div>
  );
};

export default CurrentLocation;

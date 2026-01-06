import { useState, useEffect } from 'react';
import type { GeoPosition } from '../types';

export const useGeolocation = () => {
  const [position, setPosition] = useState<GeoPosition | null>(null);
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    // Check if the Geolocation API is available in the browser.
    if (!navigator.geolocation) {
      setError('Geolocation is not supported by your browser.');
      return;
    }

    const onSuccess = (pos: GeolocationPosition) => {
      setPosition({
        coords: {
          latitude: pos.coords.latitude,
          longitude: pos.coords.longitude,
          accuracy: pos.coords.accuracy,
        }
      });
      setError(null);
    };

    const onError = (err: GeolocationPositionError) => {
      setError(`Geolocation error: ${err.message}`);
    };

    // Use watchPosition for continuous, real-time updates.
    const watchId = navigator.geolocation.watchPosition(onSuccess, onError, {
        enableHighAccuracy: true,
        timeout: 10000,
        maximumAge: 0, // Request a fresh position
    });

    // Cleanup function: This will be called when the component unmounts.
    // It's important to clear the watch to prevent memory leaks.
    return () => {
      navigator.geolocation.clearWatch(watchId);
    };
  }, []); // The empty dependency array ensures this effect runs only once on mount.

  return { position, error };
};

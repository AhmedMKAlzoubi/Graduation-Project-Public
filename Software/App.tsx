
import React, { useState, useEffect, useCallback } from 'react';
import { DirectionStep, PlaceSuggestion, Route } from './types';
import { getRoute } from './services/navigationService';
import { getPlaceSuggestions } from './services/geminiService';
import { useBLE } from './hooks/useBLE';
import { BleService } from './plugins/bleService';
import { useGeolocation } from './hooks/useGeolocation';
import { useHelmetDataSender } from './hooks/useHelmetDataSender';

import DestinationInput from './components/DestinationInput';
import DirectionsPanel from './components/DirectionsPanel';
import BleConnectButton from './components/BleConnectButton';
import BleDevicePicker from './components/BleDevicePicker';
import AiSuggestions from './components/AiSuggestions';
import CurrentLocation from './components/CurrentLocation';
import Map from './components/Map';
import SimulationControls from './components/SimulationControls';

// Example BLE Service and Characteristic UUIDs.
// These should match the UUIDs programmed on your ESP32 device.
const HELMET_SERVICE_UUID = "19b10000-e8f2-537e-4f6c-d104768a1214";
const DIRECTION_CHARACTERISTIC_UUID = "19b10001-e8f2-537e-4f6c-d104768a1214";

// Haversine formula to calculate distance between two lat/lng points in meters
const getDistance = (
  pos1: { lat: number; lng: number },
  pos2: { lat: number; lng: number }
): number => {
  const R = 6371e3; // Earth's radius in meters
  const rad = Math.PI / 180;
  const lat1 = pos1.lat * rad;
  const lat2 = pos2.lat * rad;
  const deltaLat = (pos2.lat - pos1.lat) * rad;
  const deltaLng = (pos2.lng - pos1.lng) * rad;

  const a =
    Math.sin(deltaLat / 2) * Math.sin(deltaLat / 2) +
    Math.cos(lat1) * Math.cos(lat2) *
    Math.sin(deltaLng / 2) * Math.sin(deltaLng / 2);
  const c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1 - a));

  return R * c;
};

export default function App() {
  // State for Splash Screen
  const [showSplash, setShowSplash] = useState(true);

  const [destination, setDestination] = useState<string>('');
  const [route, setRoute] = useState<Route | null>(null);
  const [currentStepIndex, setCurrentStepIndex] = useState(0);
  const [suggestions, setSuggestions] = useState<PlaceSuggestion[]>([]);
  const [isLoading, setIsLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [distanceToNextStep, setDistanceToNextStep] = useState<number | null>(null);
  const [isMuted, setIsMuted] = useState(false); // State for voice mute/unmute
  const [isSendPaused, setIsSendPaused] = useState(false);
  const [resumeDelayUntil, setResumeDelayUntil] = useState<number | null>(null);

  const { device, isConnected, isConnecting, devices, connectedAt, connect, connectToDevice, disconnect, writeDirection } = useBLE(HELMET_SERVICE_UUID, DIRECTION_CHARACTERISTIC_UUID);
  const [showPicker, setShowPicker] = useState(false);
  const { position, error: geoError } = useGeolocation();

  const currentStep = route ? route.steps[currentStepIndex] : null;

  // Effect to handle Splash Screen timer
  useEffect(() => {
    const timer = setTimeout(() => {
      setShowSplash(false);
    }, 3000);
    return () => clearTimeout(timer);
  }, []);

  useEffect(() => {
    try {
      if (typeof navigator !== 'undefined' && navigator.geolocation) {
        navigator.geolocation.getCurrentPosition(() => {}, () => {}, { enableHighAccuracy: true, timeout: 5000 });
      }
    } catch {}
  }, []);

  // Effect to speak the current direction.
  useEffect(() => {
    const hasSpeech = typeof window !== 'undefined' && 'speechSynthesis' in window;
    if (!hasSpeech) {
      return;
    }
    if (isMuted || !currentStep) {
      try { window.speechSynthesis.cancel(); } catch {}
      return;
    }
    const utterance = new SpeechSynthesisUtterance(currentStep.instruction);
    utterance.lang = 'en-US';
    utterance.rate = 1.0;
    try {
      window.speechSynthesis.cancel();
      window.speechSynthesis.speak(utterance);
    } catch {}
    return () => {
      try { window.speechSynthesis.cancel(); } catch {}
    };
  }, [currentStep, isMuted]);

  // Use the custom hook to handle sending data to the smart helmet.
  useHelmetDataSender({
    isConnected,
    currentStep,
    distanceToNextStep,
    writeDirection,
    paused: isLoading || isConnecting || isSendPaused || (resumeDelayUntil !== null && Date.now() < resumeDelayUntil),
    connectedAt,
    currentPosition: position,
  });

  useEffect(() => {
    const start = async () => {
      if (isConnected && device && device.deviceId) {
        try {
          await BleService.start({ deviceId: device.deviceId, serviceUuid: HELMET_SERVICE_UUID, characteristicUuid: DIRECTION_CHARACTERISTIC_UUID });
        } catch {}
      }
    };
    start();
    return () => {};
  }, [isConnected, device]);
  
  // Effect to automatically advance navigation steps based on geolocation.
  useEffect(() => {
    if (!position || !route || !currentStep) {
      setDistanceToNextStep(null);
      return; // No need to check if navigation isn't active.
    }

    const currentCoords = {
      lat: position.coords.latitude,
      lng: position.coords.longitude,
    };
    
    const stepEndLocation = currentStep.endLocation;
    const distanceToNextTurn = getDistance(currentCoords, stepEndLocation);
    setDistanceToNextStep(distanceToNextTurn);

    // Don't auto-advance on the final "You have arrived" step.
    if (currentStepIndex === route.steps.length - 1) {
      return;
    }

    // Threshold for advancing: 25 meters or current GPS accuracy, whichever is larger.
    const threshold = Math.max(25, position.coords.accuracy);

    if (distanceToNextTurn < threshold) {
      console.log(`Step ${currentStepIndex} completed (distance: ${distanceToNextTurn.toFixed(1)}m < threshold: ${threshold.toFixed(1)}m). Advancing to next step.`);
      setCurrentStepIndex(prev => prev + 1);
    }
  }, [position, route, currentStep, currentStepIndex]);


  const handleSearch = useCallback(async (newDestination: string) => {
    if (!newDestination || !position) {
      setError("Your current location is not available. Please enable location services and try again.");
      return;
    }
    
    setIsLoading(true);
    setError(null);
    setRoute(null);
    setSuggestions([]);
    setCurrentStepIndex(0);
    setDestination(newDestination);

    try {
      const origin = { lat: position.coords.latitude, lng: position.coords.longitude };
      // Fetch route and AI suggestions in parallel
      const [routeData, suggestionsData] = await Promise.all([
        getRoute(origin, newDestination),
        getPlaceSuggestions(newDestination)
      ]);
      
      setRoute(routeData);
      setSuggestions(suggestionsData);
    } catch (err: any) {
      console.error("Failed to fetch route or suggestions:", err);
      setError(err.message || "Could not fetch navigation data. Please try again.");
    } finally {
      setIsLoading(false);
    }
  }, [position]);

  // Effect to handle incoming shared destinations from the URL.
  useEffect(() => {
    const checkSharedDestination = async () => {
      // Wait for geolocation to be available before processing the share.
      if (!position) return;
      
      const params = new URLSearchParams(window.location.search);
      // 'text' is commonly used for the main shared content, 'title' is a fallback.
      const sharedDestination = params.get('text') || params.get('title');
      
      if (sharedDestination) {
        // Clean the URL to prevent re-triggering on reload.
        window.history.replaceState({}, document.title, window.location.pathname);
        // Automatically start the search.
        await handleSearch(sharedDestination);
      }
    };

    checkSharedDestination();
  }, [position, handleSearch]); // Depends on position to ensure we have an origin.

  const resetNavigation = () => {
    setDestination('');
    setRoute(null);
    setSuggestions([]);
    setCurrentStepIndex(0);
    setError(null);
    setDistanceToNextStep(null);
  };
  
  const handlePrevStep = () => {
    setCurrentStepIndex(prev => Math.max(0, prev - 1));
  };

  const handleNextStep = () => {
    if (route) {
      setCurrentStepIndex(prev => Math.min(route.steps.length - 1, prev + 1));
    }
  };

  const toggleMute = () => {
    setIsMuted(prev => !prev);
  };
  
  const handleNavigateToSuggestion = (suggestion: PlaceSuggestion) => {
    handleSearch(suggestion.name);
  };

  // RENDER SPLASH SCREEN
  if (showSplash) {
    return (
      <div className="fixed inset-0 bg-gray-900 flex flex-col items-center justify-center z-50">
        <div className="animate-bounce mb-8">
            {/* Custom Helmet Logo */}
            <svg width="140" height="140" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round" className="text-cyan-400">
                <path d="M12 2a9 9 0 0 0-9 9v7a3 3 0 0 0 3 3h12a3 3 0 0 0 3-3v-7a9 9 0 0 0-9-9z"/>
                <path d="M3 11h18"/>
                <path d="M12 2v9"/>
                <rect x="7" y="11" width="10" height="6" rx="1" />
            </svg>
        </div>
        <h1 className="text-4xl md:text-6xl font-black text-white tracking-widest">
            SMART <span className="text-cyan-400">HELMET</span>
        </h1>
        <div className="mt-8 flex items-center gap-3">
            <div className="w-2 h-2 bg-cyan-500 rounded-full animate-ping"></div>
            <span className="text-cyan-500 font-mono text-sm tracking-widest">INITIALIZING SYSTEM</span>
        </div>
      </div>
    );
  }

  return (
    <div className="min-h-screen bg-gray-900 text-gray-100 flex flex-col font-sans">
      <header className="bg-gray-800/50 backdrop-blur-sm p-4 shadow-lg sticky top-0 z-10 flex flex-wrap items-center justify-between gap-4">
        <h1 className="text-xl font-bold text-cyan-400">Smart Helmet Navigation</h1>
        <div className="flex items-center gap-3">
          <button
            onClick={() => {
              setIsSendPaused(p => {
                const next = !p;
                if (!next) {
                  setResumeDelayUntil(Date.now() + 3000);
                }
                return next;
              });
            }}
            className={`px-3 py-2 rounded-lg text-white font-semibold ${isSendPaused ? 'bg-yellow-600 hover:bg-yellow-700' : 'bg-green-600 hover:bg-green-700'}`}
          >
            {isSendPaused ? 'Resume Helmet Updates' : 'Pause Helmet Updates'}
          </button>
          
          <BleConnectButton
            isConnected={isConnected}
            isConnecting={isConnecting}
            onConnect={() => { setShowPicker(true); connect(); }}
            onDisconnect={async () => { try { await BleService.stop(); } catch {} await disconnect(); }}
          />
        </div>
      </header>
      
      <main className="flex-grow container mx-auto p-4 md:p-6 flex flex-col gap-6 max-w-2xl">
        <CurrentLocation position={position} />

        <div className="bg-gray-800 p-6 rounded-lg shadow-2xl">
            {!route ? (
              <DestinationInput onSearch={handleSearch} isLoading={isLoading || !position} />
            ) : (
               <div className="text-center">
                 <h2 className="text-lg font-semibold mb-2">Navigating to:</h2>
                 <p className="text-cyan-400 text-xl font-bold mb-4">{destination}</p>
                 <button 
                  onClick={resetNavigation}
                  className="w-full bg-red-600 hover:bg-red-700 text-white font-bold py-2 px-4 rounded-lg transition-colors"
                  >
                  End Navigation
                 </button>
               </div>
            )}
            {error && <p className="mt-4 text-red-400 text-center">{error}</p>}
        </div>
        
        {geoError && <p className="text-red-400 text-center text-sm bg-gray-800 p-4 rounded-lg">{geoError}</p>}

        {route && (
            <Map route={route} position={position}/>
        )}

        {currentStep && (
            <DirectionsPanel
              step={currentStep}
              liveDistance={distanceToNextStep}
              isMuted={isMuted}
              onToggleMute={toggleMute}
            />
        )}

        {route && (
          <SimulationControls
            onPrev={handlePrevStep}
            onNext={handleNextStep}
            isAtStart={currentStepIndex === 0}
            isAtEnd={route.steps.length > 0 && currentStepIndex === route.steps.length - 1}
          />
        )}

        {suggestions.length > 0 && <AiSuggestions suggestions={suggestions} onNavigateToSuggestion={handleNavigateToSuggestion} />}
        {showPicker && !isConnected && (
          <BleDevicePicker
            devices={devices}
            onConnect={(id) => { setShowPicker(false); connectToDevice(id); }}
            onClose={() => setShowPicker(false)}
          />
        )}
      </main>
    </div>
  );
}

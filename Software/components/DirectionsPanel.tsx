
import React from 'react';
import { DirectionStep } from '../types';
import { 
    ArrowUp, CornerUpLeft, CornerUpRight, Volume2, VolumeX,
    TurnSlightLeft, TurnSlightRight, RampLeft, RampRight, RoundaboutLeft, RoundaboutRight, ForkLeft, ForkRight, Merge 
} from './Icons';

interface DirectionsPanelProps {
  step: DirectionStep;
  liveDistance?: number | null;
  isMuted: boolean;
  onToggleMute: () => void;
}

const ManeuverIcon: React.FC<{ maneuver: string }> = ({ maneuver }) => {
    const iconClass = "w-12 h-12 md:w-16 md:h-16 text-cyan-400";
    switch (maneuver) {
        case 'turn-sharp-left':
        case 'uturn-left':
        case 'turn-left':
            return <CornerUpLeft className={iconClass} />;
        case 'turn-slight-left':
            return <TurnSlightLeft className={iconClass} />;
        case 'turn-sharp-right':
        case 'uturn-right':
        case 'turn-right':
            return <CornerUpRight className={iconClass} />;
        case 'turn-slight-right':
            return <TurnSlightRight className={iconClass} />;
        case 'ramp-left':
        case 'exit-left':
            return <RampLeft className={iconClass} />;
        case 'ramp-right':
        case 'exit-right':
            return <RampRight className={iconClass} />;
        case 'roundabout-left':
            return <RoundaboutLeft className={iconClass} />;
        case 'roundabout-right':
            return <RoundaboutRight className={iconClass} />;
        case 'fork-left':
        case 'stay-left':
            return <ForkLeft className={iconClass} />;
        case 'fork-right':
        case 'stay-right':
            return <ForkRight className={iconClass} />;
        case 'merge':
            return <Merge className={iconClass} />;
        case 'straight':
        case 'stay-straight':
        default:
            return <ArrowUp className={iconClass} />;
    }
};


const DirectionsPanel: React.FC<DirectionsPanelProps> = ({ step, liveDistance, isMuted, onToggleMute }) => {
  const formatDistance = (meters: number): string => {
    if (meters >= 1000) {
      return `${(meters / 1000).toFixed(1)} km`;
    }
    return `${Math.round(meters)} m`;
  };
  
  const displayDistance = (liveDistance !== null && typeof liveDistance !== 'undefined')
    ? formatDistance(liveDistance)
    : step.distance;

  return (
    <div className="bg-gray-800 p-4 sm:p-6 rounded-lg shadow-2xl flex items-center gap-4 sm:gap-6 border-t-4 border-cyan-500">
      <div className="flex-shrink-0">
        <ManeuverIcon maneuver={step.maneuver} />
      </div>
      <div className="flex-grow">
        <p className="text-xl md:text-3xl font-bold text-white">{step.instruction}</p>
        <p className="text-lg md:text-2xl text-gray-300 mt-1">{displayDistance}</p>
      </div>
      <div className="flex-shrink-0">
        <button 
          onClick={onToggleMute} 
          className="p-2 rounded-full hover:bg-gray-700 transition-colors focus:outline-none focus:ring-2 focus:ring-cyan-500"
          aria-label={isMuted ? "Unmute voice directions" : "Mute voice directions"}
        >
          {isMuted ? (
            <VolumeX className="w-6 h-6 text-gray-400" />
          ) : (
            <Volume2 className="w-6 h-6 text-white" />
          )}
        </button>
      </div>
    </div>
  );
};

export default DirectionsPanel;

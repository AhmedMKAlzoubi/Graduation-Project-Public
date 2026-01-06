
import React from 'react';
import { ArrowLeft, ArrowRight } from './Icons';

interface SimulationControlsProps {
  onPrev: () => void;
  onNext: () => void;
  isAtStart: boolean;
  isAtEnd: boolean;
}

const SimulationControls: React.FC<SimulationControlsProps> = ({ onPrev, onNext, isAtStart, isAtEnd }) => {
  return (
    <div className="bg-gray-800 p-4 rounded-lg shadow-2xl text-center">
      <h3 className="text-md font-semibold mb-3 text-gray-400">Navigation Simulation</h3>
      <div className="flex justify-center gap-4">
        <button
          onClick={onPrev}
          disabled={isAtStart}
          className="flex items-center gap-2 bg-gray-600 hover:bg-gray-700 text-white font-bold py-2 px-4 rounded-lg transition-colors disabled:opacity-50 disabled:cursor-not-allowed"
        >
          <ArrowLeft className="w-5 h-5" />
          Previous
        </button>
        <button
          onClick={onNext}
          disabled={isAtEnd}
          className="flex items-center gap-2 bg-gray-600 hover:bg-gray-700 text-white font-bold py-2 px-4 rounded-lg transition-colors disabled:opacity-50 disabled:cursor-not-allowed"
        >
          Next
          <ArrowRight className="w-5 h-5" />
        </button>
      </div>
    </div>
  );
};

export default SimulationControls;

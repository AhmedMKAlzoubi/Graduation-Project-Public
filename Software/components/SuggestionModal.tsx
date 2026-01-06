
import React, { useEffect } from 'react';
import { PlaceSuggestion } from '../types';

interface SuggestionModalProps {
  suggestion: PlaceSuggestion;
  onConfirm: () => void;
  onClose: () => void;
}

const SuggestionModal: React.FC<SuggestionModalProps> = ({ suggestion, onConfirm, onClose }) => {
  // Handle Escape key to close
  useEffect(() => {
    const handleKeyDown = (event: KeyboardEvent) => {
      if (event.key === 'Escape') {
        onClose();
      }
    };
    window.addEventListener('keydown', handleKeyDown);
    return () => {
      window.removeEventListener('keydown', handleKeyDown);
    };
  }, [onClose]);

  return (
    <div
      className="fixed inset-0 bg-black bg-opacity-75 flex justify-center items-center z-50"
      aria-labelledby="modal-title"
      role="dialog"
      aria-modal="true"
      onClick={onClose} // Close on overlay click
    >
      <div
        className="bg-gray-800 rounded-lg shadow-xl p-6 m-4 max-w-sm w-full border border-gray-700"
        onClick={(e) => e.stopPropagation()} // Prevent closing when clicking inside the modal
      >
        <h2 id="modal-title" className="text-xl font-bold text-white mb-2">
          Change Destination?
        </h2>
        <p className="text-gray-300 mb-4">
          Would you like to start navigating to <strong className="text-cyan-400">{suggestion.name}</strong> instead?
        </p>
        <div className="flex justify-end gap-4">
          <button
            onClick={onClose}
            className="px-4 py-2 bg-gray-600 hover:bg-gray-700 text-white font-semibold rounded-lg transition-colors focus:outline-none focus:ring-2 focus:ring-gray-500"
          >
            Cancel
          </button>
          <button
            onClick={onConfirm}
            className="px-4 py-2 bg-cyan-600 hover:bg-cyan-700 text-white font-semibold rounded-lg transition-colors focus:outline-none focus:ring-2 focus:ring-cyan-500"
          >
            Navigate
          </button>
        </div>
      </div>
    </div>
  );
};

export default SuggestionModal;

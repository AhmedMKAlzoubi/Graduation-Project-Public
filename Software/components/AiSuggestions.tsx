
import React, { useState } from 'react';
import type { PlaceSuggestion } from '../types';
import { Coffee, Utensils, Zap, TreePalm } from './Icons';
import SuggestionModal from './SuggestionModal';

const CategoryIcon: React.FC<{ category: PlaceSuggestion['category'] }> = ({ category }) => {
    const className = "w-8 h-8 text-cyan-400";
    switch(category) {
        case 'Coffee Shop': return <Coffee className={className}/>;
        case 'Restaurant': return <Utensils className={className}/>;
        case 'Gas Station': return <Zap className={className} />;
        case 'Park': return <TreePalm className={className} />;
        default: return <Zap className={className} />;
    }
};

interface AiSuggestionsProps {
    suggestions: PlaceSuggestion[];
    onNavigateToSuggestion: (suggestion: PlaceSuggestion) => void;
}

const AiSuggestions: React.FC<AiSuggestionsProps> = ({ suggestions, onNavigateToSuggestion }) => {
  const [selectedSuggestion, setSelectedSuggestion] = useState<PlaceSuggestion | null>(null);

  const handleSuggestionClick = (suggestion: PlaceSuggestion) => {
    setSelectedSuggestion(suggestion);
  };

  const handleConfirmNavigation = () => {
    if (selectedSuggestion) {
      onNavigateToSuggestion(selectedSuggestion);
      setSelectedSuggestion(null); // Close modal after confirming
    }
  };

  const handleCloseModal = () => {
    setSelectedSuggestion(null);
  };
    
  return (
    <>
        <div className="bg-gray-800 p-6 rounded-lg shadow-2xl">
        <h3 className="text-xl font-bold mb-4 text-gray-200 border-b-2 border-gray-700 pb-2">AI Suggestions Near Destination</h3>
        <ul className="space-y-4">
            {suggestions.map((suggestion, index) => (
            <li key={index}>
                <button
                onClick={() => handleSuggestionClick(suggestion)}
                className="w-full flex items-start gap-4 p-3 bg-gray-700/50 rounded-lg text-left hover:bg-gray-700 transition-colors focus:outline-none focus:ring-2 focus:ring-cyan-500"
                aria-label={`Get directions to ${suggestion.name}`}
                >
                <div className="flex-shrink-0 pt-1">
                    <CategoryIcon category={suggestion.category} />
                </div>
                <div>
                    <h4 className="font-semibold text-white">{suggestion.name}</h4>
                    <p className="text-sm text-gray-400">{suggestion.description}</p>
                </div>
                </button>
            </li>
            ))}
        </ul>
        </div>
        
        {selectedSuggestion && (
            <SuggestionModal
                suggestion={selectedSuggestion}
                onConfirm={handleConfirmNavigation}
                onClose={handleCloseModal}
            />
        )}
    </>
  );
};

export default AiSuggestions;

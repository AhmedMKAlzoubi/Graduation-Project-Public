// Represents a single turn-by-turn instruction in a navigation route.
export interface DirectionStep {
  instruction: string; // e.g., "Turn left onto Main St"
  distance: string;    // e.g., "500 ft"
  maneuver: string;    // e.g., "turn-left"
  endLocation: { lat: number; lng: number };
}

// Represents a complete navigation route from a start point to a destination.
export interface Route {
  summary: string;     // e.g., "15 min (4.3 miles)"
  steps: DirectionStep[];
  directionsResult: any | null; // Holds the raw google.maps.DirectionsResult object, or null for mocked data
}

// Represents an AI-generated suggestion for a place of interest.
export interface PlaceSuggestion {
  name: string;
  category: 'Coffee Shop' | 'Gas Station' | 'Restaurant' | 'Park' | 'Attraction' | 'Other';
  description: string;
}

// Represents a geographic position with latitude and longitude.
export interface GeoPosition {
    coords: {
        latitude: number;
        longitude: number;
        accuracy: number;
    };
}

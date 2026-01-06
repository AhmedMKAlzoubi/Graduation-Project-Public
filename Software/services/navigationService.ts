import type { Route, DirectionStep } from '../types';

// Fix: Declare google property on window to fix TypeScript errors
declare global {
  interface Window {
    google: any;
  }
}

/**
 * == REAL IMPLEMENTATION (Currently Active) ==
 * Fetches a route from the Google Maps Directions Service.
 * To enable this, comment out the DEMO MODE function above and uncomment this one.
 * Make sure your Google Cloud project has both 'Maps JavaScript API' and 'Directions API' enabled.
 */
const parseDirectionsResult = (result: any): Route => {
    if (!result.routes || result.routes.length === 0) {
        throw new Error("No routes found.");
    }

    const leg = result.routes[0].legs[0];
    const steps: DirectionStep[] = leg.steps.map((step: any) => ({
        instruction: step.instructions.replace(/<[^>]*>/g, ''), // Strip HTML tags
        distance: step.distance?.text || '',
        maneuver: step.maneuver || 'straight', // Google's maneuver strings are often compatible
        endLocation: { lat: step.end_location.lat(), lng: step.end_location.lng() },
    }));

    return {
        summary: `${leg.duration?.text} (${leg.distance?.text})`,
        steps,
        directionsResult: result, // Pass the raw result for the map renderer
    };
};

export const getRoute = (
    origin: { lat: number, lng: number },
    destination: string
): Promise<Route> => {
  console.log(`Fetching real route from ${JSON.stringify(origin)} to ${destination}...`);
  
  return new Promise((resolve, reject) => {
    // Ensure the Google Maps API script has loaded.
    if (!window.google || !window.google.maps?.DirectionsService) {
        return reject(new Error("Google Maps API is not available. Check your API key in index.html."));
    }

    const directionsService = new (window as any).google.maps.DirectionsService();

    directionsService.route(
      {
        origin: origin,
        destination: destination,
        travelMode: (window as any).google.maps.TravelMode.DRIVING,
      },
      (result: any, status: any) => {
        if (status === 'OK' && result) {
            try {
                const parsedRoute = parseDirectionsResult(result);
                resolve(parsedRoute);
            } catch (error) {
                reject(error);
            }
        } else {
            let errorMessage = `Directions request failed. Please try again. Status: ${status}`;
            switch (status) {
                case 'NOT_FOUND':
                    errorMessage = "Could not find the destination. Please check the address for typos and try again.";
                    break;
                case 'ZERO_RESULTS':
                    errorMessage = "No route could be found between your location and the destination.";
                    break;
                case 'REQUEST_DENIED':
                    errorMessage = "Directions request was denied. This can happen due to an incorrect API key or billing issues.";
                    break;
                case 'OVER_QUERY_LIMIT':
                    errorMessage = "The application has sent too many requests. Please try again later.";
                    break;
                case 'UNKNOWN_ERROR':
                    errorMessage = "An unknown server error occurred while fetching directions. Please try again.";
                    break;
            }
            reject(new Error(errorMessage));
        }
      }
    );
  });
};

/**
 * Converts latitude and longitude coordinates into a human-readable address.
 * @param coords The latitude and longitude to reverse geocode.
 * @returns A promise that resolves to a formatted address string.
 */
export const getAddressFromCoordinates = (coords: { lat: number, lng: number }): Promise<string> => {
    return new Promise((resolve, reject) => {
        if (!window.google || !window.google.maps?.Geocoder) {
            return reject(new Error("Google Maps Geocoder is not available."));
        }

        const geocoder = new window.google.maps.Geocoder();
        geocoder.geocode({ location: coords }, (results: any, status: any) => {
            if (status === 'OK' && results && results[0]) {
                resolve(results[0].formatted_address);
            } else {
                reject(new Error(`Geocoder failed due to: ${status}`));
            }
        });
    });
};
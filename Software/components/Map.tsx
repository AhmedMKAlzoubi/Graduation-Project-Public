import React, { useEffect, useRef } from 'react';
import { GeoPosition, Route } from '../types';

interface MapProps {
    route: Route | null;
    position: GeoPosition | null;
}

// Custom dark theme for Google Maps to match the app's aesthetic
const mapStyles = [
    { elementType: "geometry", stylers: [{ color: "#242f3e" }] },
    { elementType: "labels.text.stroke", stylers: [{ color: "#242f3e" }] },
    { elementType: "labels.text.fill", stylers: [{ color: "#746855" }] },
    {
      featureType: "administrative.locality",
      elementType: "labels.text.fill",
      stylers: [{ color: "#d59563" }],
    },
    {
      featureType: "poi",
      elementType: "labels.text.fill",
      stylers: [{ color: "#d59563" }],
    },
    {
      featureType: "poi.park",
      elementType: "geometry",
      stylers: [{ color: "#263c3f" }],
    },
    {
      featureType: "poi.park",
      elementType: "labels.text.fill",
      stylers: [{ color: "#6b9a76" }],
    },
    {
      featureType: "road",
      elementType: "geometry",
      stylers: [{ color: "#38414e" }],
    },
    {
      featureType: "road",
      elementType: "geometry.stroke",
      stylers: [{ color: "#212a37" }],
    },
    {
      featureType: "road",
      elementType: "labels.text.fill",
      stylers: [{ color: "#9ca5b3" }],
    },
    {
      featureType: "road.highway",
      elementType: "geometry",
      stylers: [{ color: "#746855" }],
    },
    {
      featureType: "road.highway",
      elementType: "geometry.stroke",
      stylers: [{ color: "#1f2835" }],
    },
    {
      featureType: "road.highway",
      elementType: "labels.text.fill",
      stylers: [{ color: "#f3d19c" }],
    },
    {
      featureType: "transit",
      elementType: "geometry",
      stylers: [{ color: "#2f3948" }],
    },
    {
      featureType: "transit.station",
      elementType: "labels.text.fill",
      stylers: [{ color: "#d59563" }],
    },
    {
      featureType: "water",
      elementType: "geometry",
      stylers: [{ color: "#17263c" }],
    },
    {
      featureType: "water",
      elementType: "labels.text.fill",
      stylers: [{ color: "#515c6d" }],
    },
    {
      featureType: "water",
      elementType: "labels.text.stroke",
      stylers: [{ color: "#17263c" }],
    },
  ];
  

const Map: React.FC<MapProps> = ({ route, position }) => {
    const mapRef = useRef<HTMLDivElement>(null);
    const mapInstance = useRef<any>(null);
    const directionsRenderer = useRef<any>(null);
    const userMarker = useRef<any>(null);
    const accuracyCircle = useRef<any>(null);

    useEffect(() => {
        if (!position) return;

        const currentPos = {
            lat: position.coords.latitude,
            lng: position.coords.longitude,
        };

        // Initialize map and components on first load with position
        if (mapRef.current && !mapInstance.current) {
            mapInstance.current = new window.google.maps.Map(mapRef.current, {
                center: currentPos,
                zoom: 16,
                styles: mapStyles,
                disableDefaultUI: true,
                zoomControl: true,
            });
            
            directionsRenderer.current = new window.google.maps.DirectionsRenderer({
                 map: mapInstance.current,
                 suppressMarkers: true, // We use our own custom marker
                 polylineOptions: {
                    strokeColor: '#06b6d4', // Cyan color for the route
                    strokeWeight: 6,
                    strokeOpacity: 0.8,
                 }
            });

            userMarker.current = new window.google.maps.Marker({
                map: mapInstance.current,
                title: "Your Location",
                icon: {
                    path: window.google.maps.SymbolPath.CIRCLE,
                    scale: 8,
                    fillColor: "#0ea5e9", // Sky blue
                    fillOpacity: 1,
                    strokeColor: "white",
                    strokeWeight: 2,
                }
            });

            accuracyCircle.current = new window.google.maps.Circle({
                map: mapInstance.current,
                radius: position.coords.accuracy,
                fillColor: "#0ea5e9",
                fillOpacity: 0.2,
                strokeColor: "#0ea5e9",
                strokeOpacity: 0.4,
                strokeWeight: 1,
            });
        }

        // Update position on subsequent renders
        if (mapInstance.current && userMarker.current && accuracyCircle.current) {
            userMarker.current.setPosition(currentPos);
            accuracyCircle.current.setCenter(currentPos);
            accuracyCircle.current.setRadius(position.coords.accuracy);
            mapInstance.current.panTo(currentPos);
        }

    }, [position]);

    useEffect(() => {
        // Update route on the map
        if (directionsRenderer.current) {
            if (route?.directionsResult) {
                directionsRenderer.current.setDirections(route.directionsResult);
            } else {
                // Clear the route from the map
                directionsRenderer.current.setDirections({routes: []});
            }
        }
    }, [route]);

    return (
        <div className="bg-gray-800 p-2 rounded-lg shadow-2xl">
            <div 
                ref={mapRef} 
                className="w-full h-64 md:h-80 rounded-md"
                aria-label="Map showing navigation route"
                role="application"
            />
        </div>
    );
};

export default Map;
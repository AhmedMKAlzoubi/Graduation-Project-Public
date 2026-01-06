import React from 'react';
import ReactDOM from 'react-dom/client';
import App from './App';

const rootElement = document.getElementById('root');
if (!rootElement) {
  throw new Error("Could not find root element to mount to");
}

// Dynamically inject the Google Maps API script
const googleMapsApiKey = import.meta.env.VITE_GOOGLE_MAPS_API_KEY;
if (!googleMapsApiKey) {
  throw new Error("Google Maps API key is missing. Check your .env.local file.");
}
const script = document.createElement('script');
script.src = `https://maps.googleapis.com/maps/api/js?key=${googleMapsApiKey}&libraries=routes,marker,geocoding`;
script.async = true;
document.head.appendChild(script);

const root = ReactDOM.createRoot(rootElement);
root.render(
  <React.StrictMode>
    <App />
  </React.StrictMode>
);

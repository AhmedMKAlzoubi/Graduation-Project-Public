
import { GoogleGenAI, Type } from '@google/genai';
import type { PlaceSuggestion } from '../types';

const apiKey = (process.env.API_KEY as string) || (process.env.GEMINI_API_KEY as string) || '';
let ai: GoogleGenAI | null = null;
const getClient = () => {
  if (!apiKey) {
    return null;
  }
  if (!ai) {
    ai = new GoogleGenAI({ apiKey });
  }
  return ai;
};

// This schema defines the expected JSON structure for the AI's response.
// This helps ensure we get consistent, parsable data from the model.
const suggestionsSchema = {
    type: Type.ARRAY,
    items: {
      type: Type.OBJECT,
      properties: {
        name: {
          type: Type.STRING,
          description: 'The specific name of the place, including its city or local address if it\'s a chain (e.g., "Starbucks on 5th Ave" or "Central Park, Anytown").',
        },
        category: {
          type: Type.STRING,
          description: 'The category of the place.',
          enum: ['Coffee Shop', 'Gas Station', 'Restaurant', 'Park', 'Attraction', 'Other'],
        },
        description: {
            type: Type.STRING,
            description: 'A brief, one-sentence description of the place.'
        }
      },
      required: ['name', 'category', 'description'],
    },
};

/**
 * Fetches AI-powered place suggestions near a given destination.
 * @param destination The destination to get suggestions for.
 * @returns A promise that resolves to an array of PlaceSuggestion objects.
 */
export const getPlaceSuggestions = async (destination: string): Promise<PlaceSuggestion[]> => {
  try {
    const client = getClient();
    if (!client) {
      return [];
    }
    const prompt = `Suggest exactly 3 interesting places (e.g., coffee shops, gas stations, restaurants) near the destination: "${destination}". For each suggestion, provide a specific name and location to make it easy to find on a map. For chain businesses, specify the location (e.g., 'Starbucks on Main St' or 'McDonald's in downtown'). Provide a variety of categories.`;
    
    const response = await client.models.generateContent({
      model: 'gemini-2.5-flash',
      contents: prompt,
      config: {
        responseMimeType: 'application/json',
        responseSchema: suggestionsSchema,
      },
    });

    const jsonText = response.text.trim();
    if (!jsonText) {
        console.warn("Gemini returned empty text for suggestions.");
        return [];
    }

    const suggestions = JSON.parse(jsonText);
    return suggestions as PlaceSuggestion[];
  } catch (error) {
    console.error('Error fetching Gemini suggestions:', error);
    // Return an empty array or throw the error, depending on desired error handling.
    return [];
  }
};

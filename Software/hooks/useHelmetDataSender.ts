
import { useEffect, useRef } from 'react';
import { BleService } from '../plugins/bleService';
import { DirectionStep } from '../types';

const parseRoundaboutExit = (instruction: string): string | null => {
  const text = (instruction || '').toLowerCase();
  const m1 = text.match(/(\d+)\s*(st|nd|rd|th)\s*exit/);
  if (m1 && m1[1]) return `RA-${m1[1]}`;
  const m2 = text.match(/exit\s*(number\s*)?(\d+)/);
  if (m2 && m2[2]) return `RA-${m2[2]}`;
  const m3 = text.match(/take\s*the\s*(\d+)\s*(st|nd|rd|th)/);
  if (m3 && m3[1]) return `RA-${m3[1]}`;
  return null;
};

const getEspSymbolForStep = (step: DirectionStep): string => {
  const m = step.maneuver || 'straight';
  switch (m) {
    case 'turn-left':
    case 'turn-sharp-left':
    case 'uturn-left':
    case 'turn-slight-left':
    case 'ramp-left':
    case 'fork-left':
    case 'exit-left':
    case 'stay-left':
      return 'L';
    case 'turn-right':
    case 'turn-sharp-right':
    case 'uturn-right':
    case 'turn-slight-right':
    case 'ramp-right':
    case 'fork-right':
    case 'exit-right':
    case 'stay-right':
      return 'R';
    case 'uturn':
      return 'U';
    default:
      break;
  }
  if (m.includes('roundabout')) {
    const ra = parseRoundaboutExit(step.instruction || '');
    if (ra) return ra.toUpperCase();
    return 'RA-2';
  }
  return 'S';
};

interface UseHelmetDataSenderProps {
  isConnected: boolean;
  currentStep: DirectionStep | null;
  distanceToNextStep: number | null;
  writeDirection: (data: string) => void;
  paused?: boolean;
  connectedAt?: number | null;
  currentPosition?: GeolocationPosition | null;
}

export const useHelmetDataSender = ({
  isConnected,
  currentStep,
  distanceToNextStep,
  writeDirection,
  paused = false,
  connectedAt = null,
  currentPosition = null,
}: UseHelmetDataSenderProps) => {
  const lastSentRef = useRef<string | null>(null);
  const lastTsRef = useRef<number>(0);
  const lastStepKeyRef = useRef<string | null>(null);
  const intervalRef = useRef<any>(null);
  const minSpacingMs = 500;
  const lastSymbolRef = useRef<string | null>(null);
  const lastDistanceRef = useRef<number | null>(null);
  useEffect(() => {
    const settle = connectedAt ? Date.now() - connectedAt >= 1000 : true;
    if (!isConnected || !currentStep) return;
    if (!settle) return;
    const key = currentStep.instruction || currentStep.maneuver;
    if (!paused) {
      if (lastStepKeyRef.current !== key) {
        lastStepKeyRef.current = key;
      }
    }
  }, [distanceToNextStep, currentStep, isConnected, paused, connectedAt]);

  useEffect(() => {
    if (!isConnected || paused) {
      if (intervalRef.current) { clearInterval(intervalRef.current); intervalRef.current = null; }
      return;
    }
    intervalRef.current = setInterval(() => {
      if (!currentStep) return;
      const dir = getEspSymbolForStep(currentStep);
      const distanceInMeters = distanceToNextStep !== null ? Math.round(distanceToNextStep) : null;
      const symbolChanged = lastSymbolRef.current !== dir;
      const distanceChanged = (lastDistanceRef.current !== null && distanceInMeters !== null)
        ? Math.abs(distanceInMeters - lastDistanceRef.current) > 5
        : false;
      if ((symbolChanged || distanceChanged) && Date.now() - lastTsRef.current >= 1000) {
        lastSymbolRef.current = dir;
        if (distanceInMeters !== null) { lastDistanceRef.current = distanceInMeters; }
        const payload = distanceInMeters !== null ? `${dir}:${distanceInMeters}` : dir;
        lastSentRef.current = payload;
        lastTsRef.current = Date.now();
        BleService.send({ payload });
      }
    }, 1000);
    return () => { if (intervalRef.current) { clearInterval(intervalRef.current); intervalRef.current = null; } };
  }, [isConnected, paused, currentStep, distanceToNextStep]);
};

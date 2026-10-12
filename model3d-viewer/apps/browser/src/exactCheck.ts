import type { ExactInterferenceRequest, InterferencePairResult } from '@aidc/model3d-contracts';

const EXACT_SERVICE_URL = '/api/exact-check';

export async function exactCheck(request: ExactInterferenceRequest, signal?: AbortSignal): Promise<InterferencePairResult> {
  const response = await fetch(`${EXACT_SERVICE_URL}/check`, {
    method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(request), signal,
  });
  if (!response.ok) throw new Error(`精确校核服务返回 ${response.status}`);
  return await response.json() as InterferencePairResult;
}

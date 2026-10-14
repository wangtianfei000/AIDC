import type { AnalysisSnapshot, ModelSnapshot } from '../model/types';

export interface RuntimeHealth {
  status: string;
  runtimeVersion: string;
  engineVersion: string;
  schemaVersion: string;
  parserBaseline: string;
}

export class ParserRuntimeError extends Error {
  constructor(
    message: string,
    public readonly status?: number,
    public readonly code?: string,
  ) {
    super(message);
  }
}

export class ParserClient {
  private activeRequestId?: string;

  constructor(public readonly baseUrl: string) {}

  async health(signal?: AbortSignal): Promise<RuntimeHealth> {
    return this.request<RuntimeHealth>('/health', { signal });
  }

  async capabilities(signal?: AbortSignal) {
    return this.request<Record<string, unknown>>('/capabilities', { signal });
  }

  async parse(uri: string, text: string, signal?: AbortSignal): Promise<ModelSnapshot> {
    await this.cancel();
    this.activeRequestId = crypto.randomUUID();
    const requestId = this.activeRequestId;
    try {
      return await this.request<ModelSnapshot>('/parse', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ uri, text, requestId }),
        signal,
      });
    } finally {
      if (this.activeRequestId === requestId) this.activeRequestId = undefined;
    }
  }

  async analyze(uri: string, text: string, signal?: AbortSignal): Promise<AnalysisSnapshot> {
    await this.cancel();
    this.activeRequestId = crypto.randomUUID();
    const requestId = this.activeRequestId;
    try {
      return await this.request<AnalysisSnapshot>('/analyze', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ uri, text, requestId }),
        signal,
      });
    } finally {
      if (this.activeRequestId === requestId) this.activeRequestId = undefined;
    }
  }

  async cancel(): Promise<void> {
    if (!this.activeRequestId) return;
    const requestId = this.activeRequestId;
    this.activeRequestId = undefined;
    try {
      await this.request('/cancel', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ requestId }),
      });
    } catch {
      // A stopped runtime or an already completed parse needs no further action.
    }
  }

  private async request<T = unknown>(path: string, init?: RequestInit): Promise<T> {
    let response: Response;
    try {
      response = await fetch(`${this.baseUrl}${path}`, init);
    } catch {
      throw new ParserRuntimeError(`无法连接 Parser Runtime：${this.baseUrl}`);
    }
    const body = await response.json().catch(() => ({}));
    if (!response.ok) {
      const error = body?.error;
      throw new ParserRuntimeError(error?.message ?? `Runtime 返回 HTTP ${response.status}`, response.status, error?.code);
    }
    return body as T;
  }
}

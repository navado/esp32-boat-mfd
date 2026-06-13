// GENERATED from espdisp-control-1.schema.json — do not edit.

export interface Espdisp {
  ViewRef?: ViewRef;
  Session?: Session;
  DeviceRecord?: DeviceRecord;
  Attach?: Attach;
  AttachAck?: AttachAck;
  Switch?: Switch;
  SwitchAck?: SwitchAck;
  Heartbeat?: Heartbeat;
  HeartbeatAck?: HeartbeatAck;
  Detach?: Detach;
  ControlState?: ControlState;
  [k: string]: unknown;
}
export interface ViewRef {
  id: string;
  title: string;
  [k: string]: unknown;
}
export interface Session {
  controllerId: string;
  name: string;
  color: string;
  lastSeen?: number;
  [k: string]: unknown;
}
export interface DeviceRecord {
  v: string;
  deviceId: string;
  name?: string;
  role: "display" | "controller" | "both";
  board?: string;
  display?: string;
  currentView: string;
  views?: ViewRef[];
  transports?: ("ip" | "ble")[];
  authRequired?: boolean;
  [k: string]: unknown;
}
export interface Attach {
  v: string;
  t: "attach";
  controllerId: string;
  name: string;
  color: string;
  key?: string;
  ttlMs?: number;
  [k: string]: unknown;
}
export interface AttachAck {
  v: string;
  t: "attachAck";
  accepted: boolean;
  sessionId?: string;
  ttlMs?: number;
  reason?: string;
  device?: DeviceRecord;
  [k: string]: unknown;
}
export interface Switch {
  v: string;
  t: "switch";
  sessionId: string;
  viewId: string;
  [k: string]: unknown;
}
export interface SwitchAck {
  v: string;
  t: "switchAck";
  ok: boolean;
  currentView?: string;
  reason?: string;
  [k: string]: unknown;
}
export interface Heartbeat {
  v: string;
  t: "heartbeat";
  sessionId: string;
  [k: string]: unknown;
}
export interface HeartbeatAck {
  v: string;
  t: "heartbeatAck";
  ok: boolean;
  ttlMs?: number;
  [k: string]: unknown;
}
export interface Detach {
  v: string;
  t: "detach";
  sessionId: string;
  [k: string]: unknown;
}
export interface ControlState {
  v: string;
  t: "controlState";
  currentView: string;
  sessions: Session[];
  [k: string]: unknown;
}

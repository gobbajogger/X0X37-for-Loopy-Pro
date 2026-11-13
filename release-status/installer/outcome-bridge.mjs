/** DnBOX opt-in installer-outcome bridge.
 *
 * Integration contract for the authorized installer writer:
 *   import { createOutcomeReporter, attachInstallerEvents } from "./outcome-bridge.js";
 *   const reporter = createOutcomeReporter({ endpoint:"/api/installer-report",
 *     catalogId, model, packageSha256, consent: () => userCheckedOptIn });
 *   attachInstallerEvents(document, reporter);
 *
 * Emit 'dnbox:installer-outcome' CustomEvent with detail.stage matching
 * the transfer/restart/boot stage. A transfer ACK is NEVER a boot event.
 * No firmware/package bytes or device identifiers are collected.
 *
 * Reporting is entirely optional. No report is a verified install or approval.
 */

const STAGES = new Set([
  "transfer-started", "transfer-completed", "restart-observed",
  "boot-observed", "failed",
]);

export function createOutcomeReporter({
  endpoint = "/api/installer-report",
  catalogId, model, packageSha256, consent,
  fetcher = (...args) => fetch(...args),
  clock = () => new Date().toISOString(),
  uuid = () => crypto.randomUUID(),
} = {}) {
  if (model !== "elite" && model !== "pro") {
    throw new TypeError("an explicit Elite/Pro selection is required");
  }
  if (typeof catalogId !== "string" || !catalogId ||
      !/^[a-f0-9]{64}$/.test(packageSha256 || "")) {
    throw new TypeError("exact catalog/build and package SHA-256 are required");
  }
  if (typeof consent !== "function") {
    throw new TypeError("consent must be a live opt-in callback");
  }
  if (typeof endpoint !== "string" || !endpoint.startsWith("/") || endpoint.startsWith("//")) {
    throw new TypeError("same-origin report endpoint required");
  }
  let sequence = 0;
  const runId = uuid();
  const observed = new Set();
  return Object.freeze({
    async report(stage, { confirmation = "" } = {}) {
      if (!STAGES.has(stage)) throw new TypeError("invalid installer outcome");
      if (!consent()) return { sent: false, reason: "no-consent" };
      if (sequence >= 20) return { sent: false, reason: "event-cap" };
      if (stage === "restart-observed" && !observed.has("transfer-completed")) {
        return { sent: false, reason: "restart-not-connected-to-transfer" };
      }
      if (stage === "boot-observed" &&
          (!observed.has("restart-observed") ||
           !["manual", "device-readback"].includes(confirmation))) {
        return { sent: false, reason: "boot-not-separately-confirmed" };
      }
      // This is still anonymous/unverified. Even readback reported here
      // does not grant the administrator verified-hardware authority.
      const report = {
        consent: true, catalog_id: catalogId, model,
        package_sha256: packageSha256, run_id: runId,
        sequence: ++sequence, observed_at: clock(), stage,
      };
      const response = await fetcher(endpoint, {
        method: "POST",
        mode: "same-origin",
        credentials: "same-origin",
        cache: "no-store",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(report),
      });
      if (!response.ok) {
        return { sent: false, reason: "receiver-rejected", status: response.status };
      }
      observed.add(stage);
      return { sent: true, stage, confidence: "anonymous-unverified",
               authorizes_hardware_approval: false };
    },
  });
}

export function attachInstallerEvents(target, reporter) {
  if (!target || typeof target.addEventListener !== "function" ||
      !reporter || typeof reporter.report !== "function") {
    throw new TypeError("event target and outcome reporter are required");
  }
  const listener = (event) => {
    const detail = event?.detail;
    if (!detail || !STAGES.has(detail.stage)) return;
    // Event callbacks never block the firmware installer. Delivery is best-effort.
    reporter.report(detail.stage, {
      confirmation: detail.bootConfirmation || "",
    }).catch(() => {});
  };
  target.addEventListener("dnbox:installer-outcome", listener);
  return () => target.removeEventListener("dnbox:installer-outcome", listener);
}

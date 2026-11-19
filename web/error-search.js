// Shared by the UI and offline regression checks. No user input is sent out.
export function candidates(input) {
  let q = input.trim().toUpperCase().replace(/^HMS[\s:_-]*/, '').replace(/^0X/, '').replace(/[\s_-]/g, '');
  if (!/^[0-9A-F]{1,16}$/.test(q)) return [];
  const out = [q];
  // Decimal MQTT error codes commonly have 8+ digits; also retain hex matching.
  if (/^\d{8,10}$/.test(q) && Number(q) <= 0xffffffff) out.push(Number(q).toString(16).toUpperCase().padStart(8, '0'));
  return [...new Set(out)];
}
export function search(rows, input, limit = 30) {
  const keys = candidates(input);
  if (!keys.length) return {rows: [], total: 0};
  const found = rows.filter(r => keys.some(k => r.code.startsWith(k)));
  found.sort((a,b) => Number(keys.includes(b.code))-Number(keys.includes(a.code)) || a.code.localeCompare(b.code));
  return {rows: found.slice(0,limit), total: found.length};
}

import assert from 'node:assert/strict';
import {FRAME_DEFAULTS,validateFrame,serializeFrame,parseFrame} from '../js/frame-config.js';
const str=serializeFrame(FRAME_DEFAULTS),parsed=parseFrame(str);
assert.equal(parsed.errors.length,0);
assert.deepEqual(parsed.settings,FRAME_DEFAULTS);
assert.match(str,/refresh_minutes=720/);
assert.match(str,/battery_calibration=1/);
for(const [config,key] of [
  [{...FRAME_DEFAULTS,low_battery_v:3.1},'low_battery_v'],
  [{...FRAME_DEFAULTS,orientation:12},'orientation'],
  [{...FRAME_DEFAULTS,quiet_hours_start:'25:00'},'Horaires'],
  [{...FRAME_DEFAULTS,critical_cutoff_v:3.5},'critical_cutoff_v']]){
  assert(validateFrame(config).errors.some(e=>e.includes(key)));
  assert.throws(()=>serializeFrame(config));
}
const override=parseFrame('# Example\nrefresh_minutes=30\nshuffle=on\nimage_fit=cover\n');
assert.equal(override.settings.refresh_minutes,30);
assert.equal(override.settings.shuffle,true);
assert.equal(override.settings.image_fit,'cover');
assert.equal(override.errors.length,0);
console.log('Frame config verified');

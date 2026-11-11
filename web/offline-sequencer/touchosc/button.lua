-- SPDX-License-Identifier: GPL-3.0-only
-- Identical script on each momentary button. All routing belongs to root.
function onValueChanged(key)
  if key == "x" then
    root:notify("input", {name=self.name, pressed=self.values.x > 0})
  end
end

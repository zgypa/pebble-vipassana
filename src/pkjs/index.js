// PebbleKit JS bridge for settings sync and config UI.
var keys = {
  COURSE_START: 10000,
  SERVICE_START: 10001,
  COURSE_TYPE: 10002,
  COURSE_ROLE: 10003,
  ROOM: 10004,
  PAGODA_CELL: 10005,
  DINING_HALL: 10006,
  CUSHION: 10007,
  REQUEST_SYNC: 10008,
};

var pendingConfigOpen = false;
var watchSyncReceived = false;

function isoToLocal(value) {
  if (!value) {
    return '';
  }
  return value.replace(' ', 'T');
}

function localToIso(value) {
  if (!value) {
    return '';
  }
  return value.replace('T', ' ');
}

function getStored(key, fallback) {
  var value = localStorage.getItem(key);
  return value !== null ? value : fallback;
}

function saveStored(key, value) {
  if (value === undefined || value === null) {
    return;
  }
  localStorage.setItem(key, value);
}

function buildConfigHtml(values) {
  return '<!doctype html>'
    + '<html><head><meta charset="utf-8">'
    + '<meta name="viewport" content="width=device-width, initial-scale=1">'
    + '<title>Pebble Vipassana</title>'
    + '<style>'
    + 'body{font-family:Arial,sans-serif;background:#f6f2ec;color:#2b2218;padding:18px;}'
    + 'h1{font-size:20px;margin:0 0 12px;}'
    + 'label{display:block;font-size:12px;text-transform:uppercase;letter-spacing:.06em;margin:16px 0 6px;}'
    + 'input,select{width:100%;padding:10px;border-radius:8px;border:1px solid #d4c7b9;background:#fff;} '
    + '.row{display:flex;gap:12px;}'
    + '.row > div{flex:1;}'
    + 'button{margin-top:20px;width:100%;padding:12px;border:0;border-radius:10px;background:#3a2f22;color:#fff;font-size:16px;cursor:pointer;}'
    + 'button:disabled{background:#999;cursor:not-allowed;}'
    + '.sync-btn{background:#6b5d4f;margin-top:10px;}'
    + '.status{text-align:center;padding:8px;margin:12px 0;border-radius:6px;font-size:14px;display:none;}'
    + '.status.show{display:block;}'
    + '.status.success{background:#d4edda;color:#155724;}'
    + '.status.info{background:#d1ecf1;color:#0c5460;}'
    + '.status.error{background:#f8d7da;color:#721c24;}'
    + '</style></head><body>'
    + '<h1>Pebble Vipassana</h1>'
    + '<div class="status" id="status"></div>'
    + '<form id="config">'
    + '<label>Course start</label>'
    + '<input type="datetime-local" name="courseStart" value="' + isoToLocal(values.courseStart) + '">' 
    + '<label>Service start</label>'
    + '<input type="datetime-local" name="serviceStart" value="' + isoToLocal(values.serviceStart) + '">' 
    + '<div class="row">'
    + '<div><label>Role</label>'
    + '<select name="courseRole">'
    + '<option value="0"' + (values.courseRole === '0' ? ' selected' : '') + '>Student</option>'
    + '<option value="1"' + (values.courseRole === '1' ? ' selected' : '') + '>Server</option>'
    + '</select></div>'
    + '<div><label>Course type</label>'
    + '<select name="courseType">'
    + '<option value="0"' + (values.courseType === '0' ? ' selected' : '') + '>10-day</option>'
    + '</select></div>'
    + '</div>'
    + '<label>Room</label>'
    + '<input type="text" name="room" value="' + values.room + '" placeholder="Room X">'
    + '<label>Pagoda cell</label>'
    + '<input type="text" name="pagoda" value="' + values.pagoda + '" placeholder="A1">'
    + '<label>Dining hall</label>'
    + '<input type="text" name="dining" value="' + values.dining + '" placeholder="B12">'
    + '<label>Dhamma hall cushion</label>'
    + '<input type="text" name="cushion" value="' + values.cushion + '" placeholder="C3">'
    + '<button type="submit" id="saveBtn">Save</button>'
    + '<button type="button" class="sync-btn" id="syncBtn">Sync from Watch</button>'
    + '</form>'
    + '<script>'
    + 'function showStatus(msg, type) {'
    + '  var status = document.getElementById("status");'
    + '  status.textContent = msg;'
    + '  status.className = "status show " + type;'
    + '}'
    + 'function hideStatus() {'
    + '  var status = document.getElementById("status");'
    + '  status.className = "status";'
    + '}'
    + 'function closeConfig(data) {'
    + '  var jsonData = JSON.stringify(data);'
    + '  var encoded = encodeURIComponent(jsonData);'
    + '  showStatus("Saving...", "info");'
    + '  var saveBtn = document.getElementById("saveBtn");'
    + '  if (saveBtn) saveBtn.disabled = true;'
    + '  try {'
    + '    document.location = "pebblejs://close#" + encoded;'
    + '  } catch(e) {'
    + '    console.error("Close failed:", e);'
    + '    setTimeout(function(){'
    + '      showStatus("Saved! Close this page manually.", "success");'
    + '    }, 500);'
    + '  }'
    + '}'
    + 'function localToIso(value) {'
    + '  if (!value) return "";'
    + '  return value.replace("T", " ");'
    + '}'
    + 'document.getElementById("config").addEventListener("submit", function(e){'
    + '  e.preventDefault();'
    + '  var data = {courseStart:localToIso(this.courseStart.value),' 
    + '    serviceStart:localToIso(this.serviceStart.value),' 
    + '    courseRole:this.courseRole.value,courseType:this.courseType.value,'
    + '    room:this.room.value,pagoda:this.pagoda.value,dining:this.dining.value,cushion:this.cushion.value};'
    + '  closeConfig(data);'
    + '});'
    + 'document.getElementById("syncBtn").addEventListener("click", function(){'
    + '  showStatus("Requesting settings from watch...", "info");'
    + '  this.disabled = true;'
    + '  try {'
    + '    window.location.href = "pebblejs://appMessage?" + encodeURIComponent(JSON.stringify({"REQUEST_SYNC":1}));'
    + '  } catch(e) {'
    + '    showStatus("Sync request sent. Reload page in a few seconds.", "info");'
    + '  }'
    + '  var self = this;'
    + '  setTimeout(function(){'
    + '    self.disabled = false;'
    + '    showStatus("Settings synced from watch. Reopen config to see changes.", "success");'
    + '  }, 2000);'
    + '});'
    + '</script>'
    + '</body></html>';
}

function openConfig() {
  console.log('Opening configuration page');
  var values = {
    courseStart: getStored('courseStart', ''),
    serviceStart: getStored('serviceStart', ''),
    courseRole: getStored('courseRole', '0'),
    courseType: getStored('courseType', '0'),
    room: getStored('room', ''),
    pagoda: getStored('pagoda', ''),
    dining: getStored('dining', ''),
    cushion: getStored('cushion', ''),
  };
  console.log('Config values from localStorage:', values);
  var html = buildConfigHtml(values);
  Pebble.openURL('data:text/html,' + encodeURIComponent(html));
}

function sendSettings(data) {
  console.log('Sending settings to watch:', data);
  var message = {};
  message[keys.COURSE_START] = data.courseStart || '';
  message[keys.SERVICE_START] = data.serviceStart || '';
  message[keys.COURSE_ROLE] = parseInt(data.courseRole || '0', 10);
  message[keys.COURSE_TYPE] = parseInt(data.courseType || '0', 10);
  message[keys.ROOM] = data.room || '';
  message[keys.PAGODA_CELL] = data.pagoda || '';
  message[keys.DINING_HALL] = data.dining || '';
  message[keys.CUSHION] = data.cushion || '';
  Pebble.sendAppMessage(message,
    function() {
      console.log('Settings sent to watch successfully');
    },
    function(e) {
      console.error('Failed to send settings to watch:', e);
    }
  );
}

Pebble.addEventListener('showConfiguration', openConfig);

Pebble.addEventListener('webviewclosed', function(e) {
  console.log('Webview closed event:', e);
  if (!e || !e.response) {
    console.log('No response data from webview');
    return;
  }
  try {
    var data = JSON.parse(decodeURIComponent(e.response));
    console.log('Parsed settings data:', data);
    saveStored('courseStart', data.courseStart || '');
    saveStored('serviceStart', data.serviceStart || '');
    saveStored('courseRole', data.courseRole || '0');
    saveStored('courseType', data.courseType || '0');
    saveStored('room', data.room || '');
    saveStored('pagoda', data.pagoda || '');
    saveStored('dining', data.dining || '');
    saveStored('cushion', data.cushion || '');
    console.log('Sending settings to watch');
    sendSettings(data);
  } catch (err) {
    console.error('Error processing webview response:', err);
  }
});

Pebble.addEventListener('appmessage', function(e) {
  console.log('Received app message from watch:', e.payload);
  var payload = e.payload || {};
  if (payload[keys.COURSE_START]) {
    console.log('Storing course start:', payload[keys.COURSE_START]);
    saveStored('courseStart', payload[keys.COURSE_START]);
  }
  if (payload[keys.SERVICE_START]) {
    console.log('Storing service start:', payload[keys.SERVICE_START]);
    saveStored('serviceStart', payload[keys.SERVICE_START]);
  }
  if (payload[keys.COURSE_ROLE] !== undefined) {
    saveStored('courseRole', String(payload[keys.COURSE_ROLE]));
  }
  if (payload[keys.COURSE_TYPE] !== undefined) {
    saveStored('courseType', String(payload[keys.COURSE_TYPE]));
  }
  if (payload[keys.ROOM] !== undefined) {
    saveStored('room', payload[keys.ROOM]);
  }
  if (payload[keys.PAGODA_CELL] !== undefined) {
    saveStored('pagoda', payload[keys.PAGODA_CELL]);
  }
  if (payload[keys.DINING_HALL] !== undefined) {
    saveStored('dining', payload[keys.DINING_HALL]);
  }
  if (payload[keys.CUSHION] !== undefined) {
    saveStored('cushion', payload[keys.CUSHION]);
  }
  watchSyncReceived = true;
  console.log('Settings synced from watch to localStorage');
});

Pebble.addEventListener('ready', function() {
  console.log('PebbleKit JS Ready!');
  var message = {};
  message[keys.REQUEST_SYNC] = 1;
  console.log('Requesting settings sync from watch');
  Pebble.sendAppMessage(message, 
    function() {
      console.log('Sync request sent successfully');
    },
    function(e) {
      console.error('Failed to send sync request:', e);
    }
  );

  // Open config if no settings exist (first time setup)
  var courseStart = getStored('courseStart', '');
  var serviceStart = getStored('serviceStart', '');
  if (!courseStart || !serviceStart) {
    console.log('No settings found, will open config after delay');
    setTimeout(function() {
      console.log('Opening config for first-time setup');
      openConfig();
    }, 1000);
  }
});

// PebbleKit JS bridge for settings sync and config UI.
var keys = {
  COURSE_START: 0,
  SERVICE_START: 1,
  COURSE_TYPE: 2,
  COURSE_ROLE: 3,
  ROOM: 4,
  PAGODA_CELL: 5,
  DINING_HALL: 6,
  CUSHION: 7,
  REQUEST_SYNC: 8,
};

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
    + 'button{margin-top:20px;width:100%;padding:12px;border:0;border-radius:10px;background:#3a2f22;color:#fff;font-size:16px;}'
    + '</style></head><body>'
    + '<h1>Pebble Vipassana</h1>'
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
    + '<button type="submit">Save</button>'
    + '</form>'
    + '<script>'
    + 'document.getElementById("config").addEventListener("submit", function(e){'
    + 'e.preventDefault();'
    + 'var data = {courseStart:localToIso(this.courseStart.value),' 
    + 'serviceStart:localToIso(this.serviceStart.value),' 
    + 'courseRole:this.courseRole.value,courseType:this.courseType.value,'
    + 'room:this.room.value,pagoda:this.pagoda.value,dining:this.dining.value,cushion:this.cushion.value};'
    + 'document.location = "pebblejs://close#" + encodeURIComponent(JSON.stringify(data));'
    + '});'
    + '</script>'
    + '</body></html>';
}

function openConfig() {
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
  var html = buildConfigHtml(values);
  Pebble.openURL('data:text/html,' + encodeURIComponent(html));
}

function sendSettings(data) {
  var message = {};
  message[keys.COURSE_START] = data.courseStart || '';
  message[keys.SERVICE_START] = data.serviceStart || '';
  message[keys.COURSE_ROLE] = parseInt(data.courseRole || '0', 10);
  message[keys.COURSE_TYPE] = parseInt(data.courseType || '0', 10);
  message[keys.ROOM] = data.room || '';
  message[keys.PAGODA_CELL] = data.pagoda || '';
  message[keys.DINING_HALL] = data.dining || '';
  message[keys.CUSHION] = data.cushion || '';
  Pebble.sendAppMessage(message);
}

Pebble.addEventListener('showConfiguration', openConfig);

Pebble.addEventListener('webviewclosed', function(e) {
  if (!e || !e.response) {
    return;
  }
  var data = JSON.parse(decodeURIComponent(e.response));
  saveStored('courseStart', data.courseStart || '');
  saveStored('serviceStart', data.serviceStart || '');
  saveStored('courseRole', data.courseRole || '0');
  saveStored('courseType', data.courseType || '0');
  saveStored('room', data.room || '');
  saveStored('pagoda', data.pagoda || '');
  saveStored('dining', data.dining || '');
  saveStored('cushion', data.cushion || '');
  sendSettings(data);
});

Pebble.addEventListener('appmessage', function(e) {
  var payload = e.payload || {};
  if (payload[keys.COURSE_START]) {
    saveStored('courseStart', payload[keys.COURSE_START]);
  }
  if (payload[keys.SERVICE_START]) {
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
});

Pebble.addEventListener('ready', function() {
  var message = {};
  message[keys.REQUEST_SYNC] = 1;
  Pebble.sendAppMessage(message);

  var courseStart = getStored('courseStart', '');
  var serviceStart = getStored('serviceStart', '');
  if (!courseStart || !serviceStart) {
    setTimeout(openConfig, 300);
  }
});

String.prototype.format = function () {
  var args = arguments;
  return this.replace(/\{\{|\}\}|\{(\d+)\}/g, function (m, n) {
    if (m == "{{") { return "{"; }
    if (m == "}}") { return "}"; }
    return args[n];
  });
};
console.log('{0} {1} {2} {3}'.format('this', 'is a', 'format', 'test'));

function getMinuteOfTime(time) {
  var d = new Date(time);
  var hour = d.getHours();
  hour = hour >= 10 ? hour : '0' + hour;
  var min = d.getMinutes();
  min = min >= 10 ? min : '0' + min;
  return hour + ':' + min;
}

function accumulateArray(dst, src) {
  dst[0] = src[0];
  for (var i = 1; i < src.length; ++i) {
    if (dst[i] == undefined) {
      dst[i] = 0;
    }
    dst[i] += src[i];
  }
}

function updateTr(tr, data_arr) {
  var tds = tr.children('td');
  console.log('tds length=' + tds.length);
  for (var i = 0; i < data_arr.length; ++i) {
    $(tds[i]).text(data_arr[i]);
  }
}

function buildNewTr(data_arr) {
  var new_tr = '<tr class="tr_latest">';
  for (var i = 0; i < data_arr.length; ++i) {
    new_tr += '<td>';
    new_tr += data_arr[i];
    new_tr += '</td>';
  }
  return new_tr;
}

function getDay(offset) {
  var d = new Date();
  d.setDate(d.getDate() + offset);
  return '{0}-{1}-{2}'.format(d.getFullYear(), d.getMonth() + 1, d.getDate());
}

function getDateStart() {
  var d = new Date();
  d.setHours(0);
  d.setMinutes(0);
  d.setSeconds(0);
  return d;
}

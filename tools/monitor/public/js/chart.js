Highcharts.setOptions({
	global: {
		useUTC: false
	}
});

function getDefaultCharOption() {
  return {
    chart: {
      zoomType: 'x',
      spacingRight: 20
    },
    title: {
      text: 'Chart Title'
    },
    subtitle: {
      text: document.ontouchstart === undefined
        ? 'Click and drag in the plot area to zoom in'
        : 'Pinch the chart to zoom in'
    },
    xAxis: {
      type: 'datetime',
      maxZoom: 60 * 60 * 1000,  // one hour
      title: {
        text: null
      }
    },
    yAxis: {
      title: {
        text: 'Number'
      }
    },
    tooltip: {
      shared: true
    },
    legend: {
      enabled: false
    },
    plotOptions: {
      area: {
        stacking: 'normal',
        lineColor: '#666666',
        lineWidth: 1,
        marker: {
          lineWidth: 1,
          lineColor: '#666666'
        }
      }
    },
    series: [
      {
        type: 'area',
        name: '',
        pointInterval: 60 * 1000, // one minute
        pointStart: 0,
        data: []
      },
    ]
  };
}

function createChart(container, option) {
  $(container).highcharts(option);
}

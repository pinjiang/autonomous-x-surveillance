function drawLine(optiondata){
    //el,title,subtext,xAxis,data,unit
    var opt = optiondata.option;
    var title = opt.title;
    var subtext = opt.subtext;
    var xAxis = opt.xAxis;
    var data = opt.data;
    var unit = opt.unit;
    //console.log(optiondata);

    var domx = document.getElementById(optiondata.el);
    var myChart = echarts.init(domx);
    
    option = {
        title : {
            text: title,
            subtext: subtext,
        },
        tooltip : {
            trigger: 'axis'
        },
        toolbox: {
            show : false,
            feature : {
                mark : {show: true},
                dataView : {show: true, readOnly: false},
                magicType : {show: true, type: ['line', 'bar']},
                restore : {show: true},
                saveAsImage : {show: true}
            }
        },
        calculable : true,
        xAxis : [
            {
                type : 'category',
                boundaryGap : false,
                data : xAxis
            }
        ],
        yAxis : [
            {
                type : 'value',
                boundaryGap: [0, '100%'],
                axisLabel : {
                    formatter: function(value){
                        return value+unit;
                    }
                }
            }
        ],
        series : [
            {
                name:title,
                type:'line',
                data:data,
                markPoint : {
                    data : [
                        {type : 'max', name: '最大值'},
                        {type : 'min', name: '最小值'}
                    ]
                },
                markLine : {
                    data : [
                        {type : 'average', name: '平均值'}
                    ]
                }
            }]
        };
        myChart.setOption(option,true);
    }


    function drawGauge(el,title,subtext,score,max,level1,level2){
        var domx = document.getElementById(el);
        var myChart = echarts.init(domx);
        option = {
            title: {
                text: title,
                subtext: subtext
                },
            tooltip : {
            formatter: "{a} <br/>{b} : {c}%"
            },
            toolbox: {
                show: false, 
                feature: {
                    restore: {},
                    saveAsImage: {}
                }
            },
            series: [{
                name: title,
                type: 'gauge',
                min: 0,
                max: max,
                axisLine: {            // 坐标轴线
                    lineStyle: {       // 属性lineStyle控制线条样式
                        color: [[level1/max, '#CD5C5C'],[level2/max, '#FFA500'],[1, '#3CB371']], 
                        width: 30
                    }
                },
                detail: {formatter:'{value}'},//格式
                data: [{value: score, name: '评分'}]
            }] 
        };
        myChart.setOption(option,true);
    }


    //堆积折线图
    function drawLines(el,opt){
        var domx = document.getElementById(el);
        var myChart = echarts.init(domx);

        option = {
            tooltip : {
                trigger: 'axis'
            },
            legend: {
                data:opt.selectedDataList
            },
            toolbox: {
                show : true,
                feature : {
                    mark : {show: true},
                    dataZoom : {show: true},
                    dataView : {show: true},
                    magicType : {show: true, type: ['line', 'bar', 'stack', 'tiled']},
                    restore : {show: true},
                    saveAsImage : {show: true}
                }
            },
            calculable : true,
            dataZoom : {
                show : true,
                realtime : true,
                start : 0,
                end : 100
            },
            xAxis : [
                {
                    type : 'category',
                    boundaryGap : false,
                    data : opt.xAxis
                }
            ],
            yAxis : [
                {
                    type : 'value'
                }
            ],
            series : opt.series
        };

        myChart.clear();
        myChart.setOption(option,true);
    }

//计算评分
function calculate_Score(success)
{
    var score = 0;
    if(success>=0 && success <0.9)
    {
        score = 0.5;
    }else if(success>=0.9 && success<0.99) {
        score = 1.5;
    }else if(success>=0.99 && success<0.999)
    {
        score = 3;
    }else if(success >=0.999 && success < 0.9995)
    {
        score = 3.5;
    }else if(success >=0.9995 && success < 0.9999)
    {
        score = 4;
    }else if(success >= 0.9999 && success < 0.99995)
    {
        score = 4.6;
    }else if(success >= 0.99995 && success < 0.99999)
    {
        score = 4.8;
    }else{
        score = 5;
    }
    return score;
}
    
//通用获取数据
function get_runtime_data(url,selectedDataList,callback){
    $.ajax({
        url : url,
        type : "GET",
        dataType : "json",
        cache:false,
        async : true,
        success:function(data)
        {
            //时间戳格式转换
            var timeStamp  = data.TimeStamp;
            var time = timeFormat(timeStamp);
            var dataCount = selectedDataList.length;
            // 评分赋值,待优化。。。。
            selectedData["score"] = 4.8;
            if(selectedData["xAxis"].length <= 60){
                selectedData["xAxis"].push(time);
                for(var i = 0; i < dataCount; i++){
                    selectedData[selectedDataList[i]].push(data[selectedDataList[i]]);
                }
               
            }else{
                selectedData["xAxis"].shift();
                for(var i = 0; i < dataCount; i++){
                    selectedData[selectedDataList[i]].shift();
                }

                selectedData["xAxis"].push(time);
                for(var i = 0; i < dataCount; i++){
                    selectedData[selectedDataList[i]].push(data[selectedDataList[i]]);
                }
                
            }
            
            callback(selectedData);    
        },
        error:function(XMLHttpRequest, errorThrown){
            //再次发送请求
            this.tryCount++;
            if (this.tryCount <= this.retryLimit) {
                //try again
                $.ajax(this);
                return;
            }          
            console.log("实时数据加载失败");
        }
    });
}

    //获取历史数据
    function get_historyFiles(listUrl,taskName,timeRange,callback){
        var startTime = timeRange.startTime.valueOf();
        var endTime = timeRange.endTime.valueOf();
        var list;//符合条件的文件名

        //根据起止时间，在list获取数据文件名
        $.ajax({
            url : listUrl,
            type : "GET",
            dataType : "json",
            cache:false,
            async : true,
            success:function(data)
            {
                //在list中筛选出选择任务的所有文件
                var files = data.Files;
                files = files.filter(function(result){return result["FileName"].split("_")[1] == taskName;});

                //查找早于开始时间的最晚生成的文件名
                var startIndex = search(files,"StartTime",startTime);
                var endIndex = search(files,"StartTime",endTime);
                
                //筛选满足时间条件的文件
                if(startIndex == -1){
                    startIndex = startIndex+1;
                }else if(startIndex == files.length){
                    startIndex = startIndex-1;
                }
                list = files.slice(startIndex,endIndex+1);
                //console.log(startIndex);
                //console.log(endIndex);
                //console.log(list);
                if(list.length == 0){
                    alert("无符合条件的历史数据！");
                }
                //返回数据
                callback(list);
            },
            error:function(){
                alert("list数据加载失败");
            }
        });
    }

    //读取数据文件，获取符合条件数据
    function load_data(list,timeRange,selectedDataList,callback){
        var startTime = timeRange.startTime.valueOf();
        var endTime = timeRange.endTime.valueOf();
        var dataCount = selectedDataList.length;
            
        for (var i = 0; i < list.length; i++) {
            //alert(list[i].FileName);
            var fileUrl = "data/history/" + list[i].FileName
            $.ajax({
                url : fileUrl,
                type : "GET",
                dataType : "json",
                cache:false,
                async : false,
                success:function(data)
                {
                    var historyData = data.data;
                    console.log(historyData);
                    if(i == 0){
                        //查找起始数据
                        var startIndex = search(historyData,"TimeStamp",startTime);
                        var endIndex = search(historyData,"TimeStamp",endTime);

                        var dataSlice = historyData.slice(startIndex+1,endIndex+1);
                        //console.log(dataSlice);
                            for(var j = 0; j < dataSlice.length; j++){
                                //时间戳格式转换
                                var timeStamp  = dataSlice[j]["TimeStamp"];
                                var time = timeFormat(timeStamp);
                                selectedData["xAxis"].push(time);

                                for(var k = 0; k < dataCount; k++){
                                    selectedData[selectedDataList[k]].push(dataSlice[j][selectedDataList[k]]);
                                }
                            }
                    }else if(i == list.length-1){
                        //查找截止数据
                        var endIndex = search(historyData,"TimeStamp",endTime);
                        var dataSlice = historyData.slice(0,endIndex+1);
                        for(var j = 0; j < dataSlice.length; j++){
                            //时间戳格式转换
                            var timeStamp  = dataSlice[j]["TimeStamp"];
                            var time = timeFormat(timeStamp);
                            selectedData["xAxis"].push(time);
                            for(var k = 0; k < dataCount; k++){
                                selectedData[selectedDataList[k]].push(dataSlice[j][selectedDataList[k]]);
                            }
                        }
                    }else{
                        for(var j = 0; j < historyData.length; j++){
                            //时间戳格式转换
                            var timeStamp  = dataSlice[j]["TimeStamp"];
                            var time = timeFormat(timeStamp);
                            selectedData["xAxis"].push(time);
                            for(var k = 0; k < dataCount; k++){
                                selectedData[selectedDataList[k]].push(dataSlice[j][selectedDataList[k]]);
                            }
                        }
                    }
            },
            error:function(data){
		console.log(data);
                alert("历史数据文件读取失败");
            }
        });
    }
         if(selectedData.xAxis.length == 0){
            alert("无符合条件的历史数据！");
        }
        callback(selectedData);
    }
        

//二分查找
function search(arr,key,data){
    var max = arr.length-1;  //最大值
    var min = 0;  //最小值
    if(data < arr[0][key]){
        return -1;
    }else if(data > arr[arr.length-1][key]){
        return arr.length;
    }else{
        while(min <= max){
            var mid = Math.floor((max+min)/2); //中间值
            if(arr[mid][key] < data){
                min = mid+1;
            }else if(arr[mid][key] > data){
                max = mid-1;
            }else{
                return mid; 
            }
        }
        return mid-1;
    }
}

//时间戳格式化
function timeFormat(timeStamp){
    var now = new Date(parseInt(timeStamp)),
    y = now.getFullYear(),
    m = now.getMonth() + 1,
    d = now.getDate();
    return y + "-" + (m < 10 ? "0" + m : m) + "-" + (d < 10 ? "0" + d : d) + " " + now.toTimeString().substr(0, 8);
}

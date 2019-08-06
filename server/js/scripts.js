var current_id = 0; // incremented id for tab2 

var url = "ws://localhost:20000/"
var webSocket;

var tree = [
  {
    text: "推土机",
    id:"3",
    type : "machinery",
    nodes: [
      {
        text   : "前视",
        type   : "camera",
        url    : "http://122.112.211.178:18554/test",
        user_id: "",
        user_pw: ""
      },
      {
        text   : "左视",
        type   : "camera",
        url    : "http://122.112.211.178:18554/test",
        user_id: "",
        user_pw: ""
      },
      {
        text   : "右视",
        type   : "camera",
        url    : "http://122.112.211.178:18554/test",
        user_id: "",
        user_pw: ""
      },
      {
        text   : "后视",
        type   : "camera",
        url    : "http://122.112.211.178:18554/test",
        user_id: "",
        user_pw: ""
      }
    ]
  },
  {
    text : "轮胎吊",
    id   : "2",
    type : "machinery"
  }, 
  {
    text : "挖机",
    id   : "3",
    type : "machinery",
    nodes: [
      {
        text   : "前视",
        type   : "camera",
        url    : "http://127.0.0.1:7001",
        user_id: "",
        user_pw: ""
      },
      {
        text   : "左视",
        type   : "camera",
        url    : "http://127.0.0.1:7002",
        user_id: "",
        user_pw: ""
      },
      {
        text   : "右视",
        type   : "camera",
        url    : "http://127.0.0.1:7003",
        user_id: "",
        user_pw: ""
      },
      {
        text   : "后视",
        type   : "camera",
        url    : "http://127.0.0.1:7004",
        user_id: "",
        user_pw: ""
      }
    ]
  }, 
];  

var td_template = "<td> <div class=\"custom-control custom-radio\"> \
<input type=\"radio\" class=\"custom-control-input\" id=\"defaultUnchecked\" name=\"defaultExampleRadios\"> \
<label class=\"custom-control-label\" for=\"defaultUnchecked\">Default unchecked</label> \
</div> </td>";

var block_template = "<div class=\"col-md-4 column\" id=\"camera_00\"> \
<a class=\"btn btn-info\" role=\"button\" data-toggle=\"collapse\" href=\"#config_00\" aria-expanded=\"false\" aria-controls=\"collapseExample\"> \
    详细配置 \
</a> \
<a class=\"btn btn-info\" role=\"button\" data-toggle=\"collapse\" href=\"#stats_00\" aria-expanded=\"false\" aria-controls=\"collapseExample\"> \
    详细统计量 \
</a> \
<button class=\"btn btn-primary\" onclick=\"openVideo(this);\">打开</button> \
<button class=\"btn btn-primary\" onclick=\"closeVideo(this);\">关闭</button> \
<div class=\"collapse\" id=\"config_00\"> \
  <p><label>名称:</label> <input type=\"text\" id=\"name_00\"/> </p>\
  <p>网络地址:</label> <input type=\"text\" id=\"url_00\"/> </p>\
  <p>用户名:</label> <input type=\"text\" id=\"username_00\"/> </p>\
  <p>密码:</label> <input type=\"text\" id=\"password_00\"/> </p>\
</div> \
<div class=\"collapse\" id=\"stats_00\"> \
  <table class=\"table table-bordered\"> \
        <thead> \
          <tr> \
            <th>字段</th> \
            <th>数值</th> \
          </tr> \
        </thead> \
        <tbody> \
          <tr> \
            <th scope=\"row\" id=\"codecs_encryption\">codecs/encryption</th> \
            <td id=\"codecs_encryption\"></td> \
          </tr> \
          <tr> \
            <th scope=\"row\" id=\"frameHeightWidthReceived\">frame resolution</th> \
            <td id=\"frameHeightWidthReceived_1\"></td> \
          </tr> \
          <tr>  \
            <th scope=\"row\" id=\"frameRateOutput\">frame rate(fps)</th> \
            <td id=\"frameRateOutput\"></td> \
          </tr> \
          <tr>  \
            <th scope=\"row\" id=\"bytesReceived\">bytes received(second/total)</th> \
            <td id=\"bytesReceived\"></td> \
          </tr> \
          <tr>  \
            <th scope=\"row\" id=\"packetsReceivedLost\">packets lost(%)</th> \
            <td id=\"packetsReceivedLost\"></td> \
          </tr> \
          <tr>  \
            <th scope=\"row\" id=\"latency\">latency(ms)</th> \
            <td id=\"latency\"></td> \
          </tr> \
          <tr>  \
            <th scope=\"row\" id=\"jitterbufferms\">jitter buffer(ms)</th> \
            <td id=\"jitterbufferms\"></td> \
          </tr> \
           <tr> \
            <th scope=\"row\" id=\"availreceivebw\">availreceivebw(bps)</th> \
            <td id=\"availreceivebw\"></td> \
          </tr> \
          </tbody> \
  </table> \
</div> \
</div>";

//获取树数据
function getTree(){
  return tree;             
}

function getNode(){
  
}
                    
function openSocket(){
  // Ensures only one connection is open at a time
  if(webSocket !== undefined && webSocket.readyState !== WebSocket.CLOSED){
    writeResponse("WebSocket is already opened.");
    return;
  }
  // Create a new instance of the websocket
  webSocket = new WebSocket(url);
                 
  /**
   * Binds functions to the listeners for the websocket.
   */
  webSocket.onopen = function(event){
  // For reasons I can't determine, onopen gets called twice
  // and the first time event.data is undefined.
  // Leave a comment if you know the answer.
  if(event.data === undefined)
    return;
 
   writeResponse(event.data);
  };
 
  webSocket.onmessage = function(event){
    writeResponse(event.data);
  };
 
  webSocket.onclose = function(event){
    writeResponse("Connection closed");
  };
}          
          
function closeSocket(){
  webSocket.close();
}

/**
 * ask background gstreamer media server to open a video
 */
function openVideo(obj) {
  var element = console.log($(obj).parent());
  var id = $(obj).parent().attr("id").split("_")[1];

  // console.log(id);
  // console.log("name_" + id);
  // console.log($("#name_" + id));
  // console.log($("#url_" + id));
  // console.log($("#username_" + id));
  // console.log($("#password_" + id));
  var name = $("#name_" + id).val();
  var url  = $("#url_" + id).val();
  var user_id = $("#username_" + id).val();
  var user_pw = $("#password_" + id).val();

	var jsonObj = {
    "type"   : "play",
    "name"   : name,
    "url"    : url,
    "user_id": user_id,
    "user_pw": user_pw,
  };
  console.log(jsonObj);
  webSocket.send(JSON.stringify(jsonObj));
}

/**
 * ask background gstreamer media server to close a video
 */
function closeVideo(obj) {
  var element = console.log($(obj).parent());
  var id = $(obj).parent().attr("id").split("_")[1];

  var name = $("#name_" + id).val();
	var jsonObj = {
    "type"   : "stop",
    "name"   : name,
  };
  console.log(jsonObj);
  webSocket.send(JSON.stringify(jsonObj));
}
 
function writeResponse(text){
  messages.innerHTML += "<br/>" + text;
}

function nextElement(element) {
  // var newElement = element.clone();
  var id = current_id + 1;
  current_id = id;

  if(id<10) id="0"+id;

  $(element).attr("id", $(element).attr("id").split("_")[0] + "_" + id);
  // var field = $('input', $(element)).attr("id");
  // $('input', $(element)).attr("id", field.split("_")[0]+"_"+id);

  $('input', $(element)).each(function(i, obj) {
    $(obj).attr("id", $(obj).attr("id").split("_")[0]+"_"+id);
    //test
  });

  $('a', $(element)).each(function(i, obj) {
    $(obj).attr("href", $(obj).attr("href").split("_")[0]+"_"+id);
    //test
  });

  $('.collapse', $(element)).each(function(i, obj) {
    $(obj).attr("id", $(obj).attr("id").split("_")[0]+"_"+id);
    //test
  });

  /* var field = $('.collapse', $(element)).attr("id");
  console.log(field);
  $('a', $(element)).attr("href", field.split("_")[0]+"_"+id);
  $('.collapse', $(element)).attr("id", field.split("_")[0]+"_"+id); */
  /* TBD Bug: stats is renamed to config */

  // $("#cameras").append(newElement);
}

function updateElement(element, item) {
  var id = $(element).attr("id").split("_")[1];

  $("#name_" + id, element).val(item.text);
  $("#url_" + id,  element).val(item.url);
  $("#username_" + id, element).val(item.user_id);
  $("#password_" + id, element).val(item.user_pw);
}

function appendElement(item) {
  var newElement = $.parseHTML(block_template);
  nextElement(newElement);
  updateElement(newElement, item);

  $("#cameras").append(newElement);
}

function updateTd() {

}

function appendTable(item) {
  var new_td = $.parseHTML(td_template);
  updateTd(new_td, item);
  console.log("append once");
  $("#table-content").append(new_td);
}

$(function(){
  onLoad();
  BindEvent();
   //页面加载
  function onLoad()
  {
       //渲染树
      $('#left-tree').treeview({
         data: getTree(),
         levels: 1,
         onNodeSelected:function(event, node){
           if( node.type == "machinery") {
             // To hide it
             $("#editUrlDiv").addClass('hidden');
             $("#editUsernameDiv").addClass('hidden');
             $("#editPasswordDiv").addClass('hidden');
           } 
           else if( node.type == "camera") {
             // To show it
             $("#editUrlDiv").removeClass('hidden');
             $("#editUsernameDiv").removeClass('hidden');
             $("#editPasswordDiv").removeClass('hidden');

             $('#editName').val(node.text);
             $('#editUrl').val(node.url);
             $('#editUsername').val(node.user_id);
             $('#editPassword').val(node.user_pw);
           }
         },
         showCheckbox:false//是否显示多选
      }); 
      
      // 更新
      jQuery.each(getTree(), (index, item) => {
           console.log(index, item);
           appendTable(item);
      });

      // 更新
      jQuery.each(getTree()[0].nodes, (index, item) => {
          appendElement(item);
      });
  }
  
  //事件注册
  function BindEvent()
  {
      //保存-新增
      $("#Save").click(function () {
          $('#addOperation-dialog').modal('hide')
                    //静态添加节点
                    var parentNode = $('#left-tree').treeview('getSelected');
                    var node = {
                        text: $('#addName').val()
                    };
                    $('#left-tree').treeview('addNode', [node, parentNode]);
                });
      }
       //保存-编辑
      $('#Edit').click(function(){
         var node = $('#left-tree').treeview('getSelected');
         var newNode={
         text:$('#editName').val()
        };
        $('#left-tree').treeview('updateNode', [ node, newNode]);
      });
      //显示-添加
      $("#btnAdd").click(function(){
         var node = $('#left-tree').treeview('getSelected');
         if (node.length == 0) {
           $.showMsgText('请选择节点');
           return;
          }
          $('#addName').val('');
          $('#addOperation-dialog').modal('show');
        });
        //显示-编辑
        $("#btnEdit").click(function(){
          var node=$('#left-tree').treeview('getSelected');
          $('#editShow').show();
        });
        //删除
        $("#btnDel").click(function(){
          var node = $('#left-tree').treeview('getSelected');
          if (node.length == 0) {
            $.showMsgText('请选择节点');
            return;
          }
          BootstrapDialog.confirm({
                    title: '提示',
                    message: '确定删除此节点?',
                    size: BootstrapDialog.SIZE_SMALL,
                    type: BootstrapDialog.TYPE_DEFAULT,
                    closable: true,
                    btnCancelLabel: '取消', 
                    btnOKLabel: '确定', 
                    callback: function (result) {
                        if(result)
                        {
                            del();
                        }
                    }
            });
          function del(){
            $('#left-tree').treeview('removeNode', [ node, { silent: true } ]);
          }
      });
      $("#btnMove").click(function(){
        $.showMsgText('更新中...');
      });

      /*-----页面pannel内容区高度自适应 start-----*/
      $(window).resize(function () {
        setCenterHeight();
      });
      setCenterHeight();
      function setCenterHeight() {
        var height = $(window).height();
        var centerHight = height - 240;
        $(".right_centent").height(centerHight).css("overflow", "auto");
      }
      /*-----页面pannel内容区高度自适应 end-----*/
});



var messages = document.getElementById("messages");
/**
 * Sends the value of the text input to the server
 */
function send(){
  var text = document.getElementById("messageinput").value;
  webSocket.send(text);
}
var current_id = 0; // incremented id for tab2 

var webSocket;

var saved_config = null;
var tree = [];  
var paras = [];

var block_template = "<div class=\"col-md-4 column\" id=\"camera_00\" align=\"center\"> <p> \
<h4 id=\"heading_00\"></h4> \
<a class=\"btn btn-info\" role=\"button\" id=\"config_00\" data-toggle=\"popover\" data-placement=\"bottom\"> \
    详细配置 \
</a> \
<a class=\"btn btn-info\" role=\"button\" id=\"stats_00\" data-toggle=\"popover\" data-placement=\"bottom\"> \
    详细统计量 \
</a> \
<button class=\"btn btn-primary\" onclick=\"openVideo(this);\">打开</button> \
<button class=\"btn btn-primary\" onclick=\"closeVideo(this);\">关闭</button> \
</p> \
</div>";

var messages ="";

function clone(obj) {
  if (null == obj || "object" != typeof obj) return obj;
  var copy = obj.constructor();
  for (var attr in obj) {
      if (obj.hasOwnProperty(attr)) copy[attr] = obj[attr];
  }
  return copy;
}

//获取树数据
function getTree(){
  return tree;             
}

function existNode(obj, text){
  var node = null;
  $.each(obj, function (index, value) { 
    if( value.text == text) {
      console.log(value.text);
      node = value;
      return false;
    }
    if (typeof value.nodes != undefined ) {
      node = existNode(value.nodes, text);
    }
    return true;
  })
  return node; 
}

function updateNode(obj, text, newnode) {
  var node = existNode(obj, text);
  if( node ) {
    for (var attr in node) {
      if (newnode.hasOwnProperty(attr)) node[attr] = newnode[attr];
    }
  }
}
                    
function openSocket(){
  var ws_server;

  // Ensures only one connection is open at a time
  if(webSocket !== undefined && webSocket.readyState !== WebSocket.CLOSED){
      writeResponse("WebSocket is already opened.");
      return;
  }

  if (window.location.protocol.startsWith ("file")) {
    ws_server = ws_server || "127.0.0.1";
  } else if (window.location.protocol.startsWith ("http")) {
    ws_server = ws_server || window.location.host;
  } else {
    throw new Error ("Don't know how to connect to the signalling server with uri" + window.location);
  }
  var ws_url = 'ws://' + ws_server + '/api/video/control'

  // Create a new instance of the websocket
  webSocket = new WebSocket(ws_url);
                 
  /**
   * Binds functions to the listeners for the websocket.
   */
  webSocket.onopen = function(event){
    alert("视频流媒体客户端连接成功", "Alert Title");
    writeResponse(event.data);  
  };

  webSocket.onerror = function(event) {
    alert("视频流媒体客户端连接失败", event);
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

  if (webSocket == undefined || webSocket.readyState == WebSocket.CLOSED ) {
    $('#connectModal').modal()
    return;
  } 

  // var element = console.log($(obj).parent());
  var id = $(obj).parent().attr("id").split("_")[1];
  var data_content_dom =  $.parseHTML($("#config_" + id).attr("data-content"));

  var name = $("#name_" + id, data_content_dom).text();
  var url = $("#url_" + id, data_content_dom).text();
  var user_id = $("#username_" + id, data_content_dom).text();
  var user_pw = $("#password_" + id, data_content_dom).text(); 

	var jsonObj = {
    "type"    : "play",
    "name"    : name,
    "url"     : url,
    "user_id" : user_id,
    "user_pw" : user_pw,
    "latency" : paras['rtsp']['latency'],
    "protocol": paras['rtsp']['proto']
  };
  console.log(jsonObj);
  webSocket.send(JSON.stringify(jsonObj));
}

/**
 * ask background gstreamer media server to close a video
 */
function closeVideo(obj) {

  if (webSocket == undefined) {
    $('#connectModal').modal()
    return;
  }

  var element = console.log($(obj).parent());
  var id = $(obj).parent().attr("id").split("_")[1];

  var data_content_dom =  $.parseHTML($("#config_" + id).attr("data-content"));
  var name =  $("#name_" + id, data_content_dom).text();

	var jsonObj = {
    "type"   : "stop",
    "name"   : name,
  };
  console.log(jsonObj);
  webSocket.send(JSON.stringify(jsonObj));
}
 
function writeResponse(text){
  messages += "<br/>" + text;
}

function nextElement(element) {
  // var newElement = element.clone();
  var id = current_id + 1;
  current_id = id;

  if(id<10) id="0"+id;

  $(element).attr("id", $(element).attr("id").split("_")[0] + "_" + id);

  $('h4', $(element)).each(function(i, obj) {
    $(obj).attr("id", $(obj).attr("id").split("_")[0]+"_"+id);
    //test
  });

  $('a', $(element)).each(function(i, obj) {
    $(obj).attr("id", $(obj).attr("id").split("_")[0]+"_"+id);
    //test
  });
}

/**
 */
function updateElement(element, item) {
  var id = $(element).attr("id").split("_")[1];
  $("#heading_" + id, element).text(item.text);
  $("#name_" + id, element).val(item.text);
  $("#url_" + id,  element).val(item.url);
  $("#username_" + id, element).val(item.user_id);
  $("#password_" + id, element).val(item.user_pw);

  var newconfig = "<form><ul><li id=\"name_" + id + "\"><span aria-hidden='true' class='icon_globe'></span>"+ item.text +"</li>" +
  "<li id=\"url_"+ id +"\"><span aria-hidden='true' class='icon_search_alt'></span>"  + item.url + "</li>" +
  "<li id=\"username_"+ id + "\"><span aria-hidden='true' class='icon_pens_alt'></span>" + item.user_id + "</li>" +
  "<li id=\"password_"+ id + "\"><span aria-hidden='true' class='icon_pens_alt'></span>" + item.user_pw + "</li>" +
  "</form>";

  var newstats = "<form><input id" + id + "='btn' type='button' value='跟踪' onclick='open_stats_url()'/></form>"

  $('#config_' + id, element).attr("data-content", newconfig);
  $('#stats_' + id, element).attr("data-content", newstats);
}

/**
 */
function appendElement(item) {
  var newElement = $.parseHTML(block_template);
  nextElement(newElement);
  updateElement(newElement, item);
  $("#cameras").append(newElement);
}

/**
 */
function appendTable(item) {
  var new_tr = "<tr> <td> <div class=\"custom-control custom-radio\"> \
  <input type=\"radio\" class=\"custom-control-input\" name=\"defaultExampleRadios\"> \
  </div> </td> <td>" + item.text +"</td>  <td> 2019/09/01 10:00 </td> <td> 在线 </td>  </tr>";
  var new_tr_dom = $.parseHTML(new_tr);
  $('#device-tbl tbody').append(new_tr_dom);
}

function title() {
  return '详细信息';
}

function open_stats_url() {
  var win = window.open('../line_gauge/line_gauge.html', '_blank');
  if (win) {
    //Browser has allowed it to be opened
    win.focus();
  } else {
    //Browser has blocked it
    alert('Please allow popups for this website');
  }
}
 
$(function(){
  onLoad();
  BindEvent();

  //页面加载
  function onLoad()
  {
    $.ajax({
      type: 'GET',
      url: '/api/video/config',
      contentType: 'application/json',
      success: function(res) {
        saved_config = jQuery.parseJSON(res);
        tree = saved_config['devices'];
        paras = saved_config['parameters'];
      }
    }).done(function () {
        //渲染树
        $('#left-tree').treeview({
          data: tree,
          levels: 1,
          onNodeSelected:function(event, node){
            if( node.type == "主设备") {
              // To hide it
              $("#editUrlDiv").addClass('hidden');
              $("#editUsernameDiv").addClass('hidden');
              $("#editPasswordDiv").addClass('hidden');
              $("#editType").val(node.type);
              $('#editName').val(node.text);
            } else if( node.type == "摄像机") {
              // To show it
              $("#editUrlDiv").removeClass('hidden');
              $("#editUsernameDiv").removeClass('hidden');
              $("#editPasswordDiv").removeClass('hidden');
              $('#editName').val(node.text);
              $("#editType").val(node.type);
              $('#editUrl').val(node.url);
              $('#editUsername').val(node.user_id);
              $('#editPassword').val(node.user_pw);
            }
          },
          showCheckbox:false//是否显示多选
        }); 
    }); 
  }
  
  //事件注册
  function BindEvent()
  {
    $('a[href="#tab1"]').on('click', function(event){
      //渲染树  
    })
  
    $('a[href="#tab2"]').on('click', function(event){
      $('#device-tbl tbody').empty();
      $('#cameras').empty();

      // 更新设备表格
      jQuery.each(getTree(), (index, item) => {
        appendTable(item);
      });

      // 更新摄像头表格
      jQuery.each(getTree()[0].nodes, (index, item) => {
        appendElement(item);
      });

      $("[data-toggle='popover']").popover( {
        html : true,  
        title: title(),  
        delay: { show:200, hide:200 },
      }
      );
    })
    
    //保存-新增
    $("#Save").click(function () {
        $('#addOperation-dialog').modal('hide')
          //静态添加节点
          var parentNode = $('#left-tree').treeview('getSelected');
          var node = {
            text: $('#addName').val(),
            type:$('#addType').val(),
            url:$('#addUrl').val(),
            user_id:$('#addUsername').val(),
            user_pw:$('#addPassword').val()
          };
          // parentNode.nodes.push(node);
          $('#left-tree').treeview('addNode', [node, parentNode]);
          getTree().find(x => x.text == parentNode[0].text).nodes.push(node);
      });
    }
    //保存-编辑
    $('#Edit').click(function(){
      var node = $('#left-tree').treeview('getSelected');
      var newNode={
        text:$('#editName').val(),
        type:$('#editType').val(),
        url:$('#editUrl').val(),
        user_id:$('#editUsername').val(),
        user_pw:$('#editPassword').val()
      };
      updateNode(getTree(), node[0].text, newNode);
      $('#left-tree').treeview('updateNode', [ node, newNode]);
      console.log({'devices': tree, 'parameters' : paras })
      $.ajax({
        type: 'POST',
        url: '/api/video/config',
        contentType: 'application/json',
        data: JSON.stringify( {'devices': tree, 'parameters' : paras } ), // access in body
      }).done(function () {
        console.log('SUCCESS');
        $.showMsgText('保存成功');
      }).fail(function (msg) {
        console.log('FAIL');
        $.showMsgText('保存失败');
      });
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

    //显示-添加
    /* $("#connect").click(function(){
      openSocket();
    }); */

    $('#connectModal').on('click', '.btn, .close', function() {
      $(this).addClass('modal-result'); // mark which button was clicked
    }).on('hidden.bs.modal', function() {
      var result = $(this).find('.modal-result').filter('.btn-primary').length > 0; // attempt to filter by what you consider the "YES" button; if it was clicked, result should be true.
      if( result == true ) {
        openSocket();
      }
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
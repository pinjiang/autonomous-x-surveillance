var url = "ws://localhost:20000/"
var webSocket;
var messages = document.getElementById("messages");

                    
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
           
/**
 * Sends the value of the text input to the server
 */
function send(){
  var text = document.getElementById("messageinput").value;
  webSocket.send(text);
}
           
function closeSocket(){
  webSocket.close();
}
 
function writeResponse(text){
  messages.innerHTML += "<br/>" + text;
}

/* $('#myTab a').click(function (e) {
  e.preventDefault()
  $(this).tab('show')
}) */

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
           $('#editName').val(node.text);
         },
      showCheckbox:false//是否显示多选
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
  //获取树数据
  function getTree(){
    var tree = [
    {
      text: "挖机1",
      id:"1",
      nodes: [
      {
        text: "前视",
        id:"2",
        nodes: [
        {
          text: "物理"
        },
        {
          text: "化学"
        }
        ]
      },
      {
        text: "左视"
      },
      {
        text: "右视"
      },
      {
        text: "后视"
      }
      ]
    },
    {
      text: "挖机2"
    },
    ];  
    return tree;             
  }
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
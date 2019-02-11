/* vim: set sts=4 sw=4 et :
 *
 * Demo Javascript app for negotiating and streaming a sendrecv webrtc stream
 * with a GStreamer app. Runs only in passive mode, i.e., responds to offers
 * with answers, exchanges ICE candidates, and streams.
 *
 * Author: Nirbheek Chauhan <nirbheek@centricular.com>
 */

// Set this to override the automatic detection in websocketServerConnect()
var ws_server;
var ws_port;

var our_id  = "660e8400-e29b-41d4-a716-446655440000";  // Set this to use a specific peer id instead of a random one
var peer_id = "550e8400-e29b-41d4-a716-446655440000";
// Override with your own STUN servers if you want
var rtc_configuration = {iceServers: [{ urls: "stun:122.112.211.178:3478" }]};
// The default constraints that will be attempted. Can be overriden by the user.
var default_constraints = {video: false, audio: false};

var mapping = {
	left_camera : 'video1',
	front_camera: 'video2',
	right_camera: 'video3',
	back_camera : 'video4'
};

var connect_attempts = 0;
var ws_conn = null;
var peer_connections = {
    front_camera: null,
	left_camera : null,
 	right_camera: null,
	back_camera : null 
};
var send_channel;
// Promise for local stream after constraints are approved by the user
var local_stream_promise = {
    front_camera: null,
	left_camera : null,
 	right_camera: null,
	back_camera : null 
};

var repeatInterval=5 * 1000;

var seq  = 0;
var role = "controller.video";
var dataChannel = null;
var remoteCandidates = [];
var candidateCount = 0;
var sendCandidateCount = 0;
var have_offer = false

function getOurId() {
    return Math.floor(Math.random() * (9000 - 10) + 10).toString();
}

var takeOver = function(msg) {
	var jsonObj = {
		"direction": "cs",
		"seq"      : seq++,
		"from"     : our_id,
		"role"     : role,
		"content": {
			"type" : "call",
			"peer" : peer_id,
		},
	};
	ws_conn.sendString(JSON.stringify(jsonObj));
};

var openPeerStreams = function(msg) {
    console.log("open peer's video streams .....");
    var jsobj = {
		"direction": "p2p",
		"seq":  1,
		"from": our_id,
		"to":   peer_id,
		"role": role,
		"content": {
			"type"   : "open",
			"streams": [] 
		}
    };
    if( $('input[id=right_camera]').is(':checked') ) {
       jsobj.content.streams.push("right_camera");
    } 
    if( $('input[id=front_camera]').is(':checked') ) {
       jsobj.content.streams.push("front_camera");
    }
    if( $('input[id=back_camera]').is(':checked') ) {
       jsobj.content.streams.push("back_camera");
    }
    if( $('input[id=left_camera]').is(':checked') ) {
       jsobj.content.streams.push("left_camera");
    }
	ws_conn.sendString(JSON.stringify(jsobj)); 
};

/* trigger video component to close peers streams */
function closePeerStreams() {
    console.log("close peer's video streams .....");
    var jsobj = {
		"direction":   "p2p",
		"seq"      :       1,
		"from"     :  our_id,
		"to"       : peer_id,
		"role"     :    role,
		"content": {
			"type"   : "open",
			"streams": [] 
		}
    };
    if( $('input[id=right_camera]').is(':checked') ) {
       jsobj.content.streams.push("right_camera");
    } 
    if( $('input[id=front_camera]').is(':checked') ) {
       jsobj.content.streams.push("front_camera");
    }
    if( $('input[id=back_camera]').is(':checked') ) {
       jsobj.content.streams.push("back_camera");
    }
    if( $('input[id=left_camera]').is(':checked') ) {
       jsobj.content.streams.push("left_camera");
    }
    ws_conn.sendString(JSON.stringify(jsobj));   
}

var handleCandidate = function(peer_connection, msg) {
    var candidate = new RTCIceCandidate(msg);
    peer_connection.addIceCandidate(candidate).then(function() {
		candidateCount++;
		console.log("add remote ice candidate. total count: " + candidateCount);
    }).catch(function(e) {
        console.log("Error: Failure during addIceCandidate()", e);
    });
};

function myGetStats(peer, callback) {
    if (!!navigator.mozGetUserMedia) {
		console.log("navigator.mozGetUserMedia");
        peer.getStats(
            function (res) {
                var items = [];
                res.forEach(function (result) {
                    items.push(result);
                });
                callback(items);
            },
            callback
        );
    } else {
		console.log("else");
        peer.getStats(function (res) {
            var items = [];
            res.result().forEach(function (result) {
                var item = {};
                result.names().forEach(function (name) {
                    item[name] = result.stat(name);
                });
                item.id = result.id;
                item.type = result.type;
                item.timestamp = result.timestamp;
                items.push(item);
            });
            callback(items);
        });
    }
};

function resetState() {
    // This will call onServerClose()
    ws_conn.close();
}

function handleIncomingError(error) {
    setError("ERROR: " + error);
    resetState();
}

function getAllVideoElements() {
    // return document.getElementById("stream");
    videoElementList = [];
    videoElementList.push(document.getElementById("video1"));
    videoElementList.push(document.getElementById("video2"));
    videoElementList.push(document.getElementById("video3"));

    return videoElementList;
}

function getVideoElement(id) {
	console.log("getVideoElement: " + mapping[id]);
	
    return document.getElementById(mapping[id]);
}

function setStatus(text) {
    console.log(text);
    var span = document.getElementById("status")
    // Don't set the status if it already contains an error
    if (!span.classList.contains('error'))
        span.textContent = text;
}

function setError(text) {
    console.error(text);
    var span = document.getElementById("status")
    span.textContent = text;
    span.classList.add('error');
}

function resetVideo() {
    // Release the webcam and mic
	for( i in local_stream_promise ) {
		if (local_stream_promise[i]) {
		    local_stream_promise[i].then(stream => {
		        if (stream) {
		            stream.getTracks().forEach(function (track) { track.stop(); });
		        }
		    });
		}
	}
    // Reset the video element and stop showing the last received frame
    // var videoElement = getVideoElement();
    // videoElement.pause();
    // videoElement.src = "";
    // videoElement.load();
    var videoElements = getAllVideoElements();
    videoElements.forEach(function(videoElement) {
        videoElement.pause();
        videoElement.src = "";
        videoElement.load();
    });
}

// Local description was set, send it to peer
function onLocalDescriptionWrapper(channel, peer_connection) {
	return function onLocalDescription(desc) {
		console.log("Got local description: " + JSON.stringify(desc));
		peer_connection.setLocalDescription(desc).then(function() {
        setStatus("Sending SDP answer");
		var jsonObj = {
			"direction": "p2p",
			"seq":  seq++,
			"from": our_id,
			"to":   peer_id,
			"role": role,
			"content": {
				"type" : peer_connection.localDescription.type,
				"index": channel,
				"sdp"  : peer_connection.localDescription.sdp
			}
		};
		ws_conn.sendString(JSON.stringify(jsonObj));
		});
	}
}

function onServerMessage(event) {
    var msg = JSON.parse(event.data);

    switch (msg.direction) {
        case "sc":
            switch (msg.content.type) {
                case "register_response":
                    if (msg.content.result == "success") {
                        console.log("Register Successful");
						// Incoming JSON signals the beginning of a call
						createCall(peer_connections);					
                    } else {
                        console.log("Register Failed");
                    }
                    break;
				case "call_response":
					if (msg.content.result == "success") {
						console.log("Call Successful");					
					} else {
						console.log("Call Failed");
					}
					break;
                case "disconnect":
                    console.log("signalling server disconnect");
					stop();
					break;
                default:
                    console.log("WARNING: Ignoring unknown msg type '" + msg.content.type + "'");
            }
            break;
        case "p2p":
            switch (msg.content.type) {
                case "offer":
                    console.log("received remote offer");
					var offer = { 'type': msg.content.type, 'sdp': msg.content.sdp };
                    peer_connections[msg.content.index].setRemoteDescription(new RTCSessionDescription(offer))
                        .then(function() {
                                have_offer = true;
                                var i = 0;
                                for (i = 0; i < remoteCandidates.length; i++) {
                                    handleCandidate(peer_connections[msg.content.index] , remoteCandidates[i]);
                                }
								local_stream_promise[msg.content.index].then((stream) => {
									setStatus("Got local stream, creating answer");
									// desc = peer_connections[msg.content.index].createAnswer(onLocalDescriptionWrapper(peer_connections[msg.content.index]));
									// onLocalDescription(desc, peer_connections[msg.content.index]);
									peer_connections[msg.content.index].createAnswer().then(onLocalDescriptionWrapper(msg.content.index, peer_connections[msg.content.index])
									).catch(setError);
								}).catch(setError);
                        }).catch(function(reason) {
                            console.log("error: " + reason);
                        });
                    break;
                case "ice":
                    if (msg.content.candidateLine) {
                        if (!have_offer) {
                            remoteCandidates.push(msg.content.candidateLine);
                        } else {
                            handleCandidate(peer_connections[msg.content.index], msg.content.candidateLine);
                        }
                    } else {
                        console.log("remote peer has no more candidates. total count: " + candidateCount);
                    }
                    break;
                /* case "candidates":
                    console.log("received remote ice candidates");
                    if (!have_answer) {
                        remoteCandidates.push(msg.content.ice);
                    } else {
                        for (i = 0; i < msg.content.ice.length; i++) {
                            console.log(msg.content.ice[i]);
                            if( msg.content.ice[i].candidate ) {
                                handleCandidate(msg.content.ice[i]);
                            }
                        }
                    }
                    break; */
				default:
					console.log("WARNING: Ignoring unknown msg type '" + msg.content.type + "'");
            }
            break;
        default:
            console.log("WARNING: Ignoring unknown direction '" + msg.direction + "'");
    }
}

function onServerClose(event) {
    setStatus('Disconnected from server');
    resetVideo();
	
	for (i in peer_connections) {
		if( peer_connections[i]) {
			peer_connections[i].close();
			peer_connections[i] = null;	
		}
	}
    // Reset after a second
    window.setTimeout(websocketServerConnect, 1000);
}

function onServerError(event) {
    setError("Unable to connect to server, did you add an exception for the certificate?")
    // Retry after 3 seconds
    window.setTimeout(websocketServerConnect, 3000);
}

function getLocalStream() {

    var constraints;
    var textarea = document.getElementById('constraints');
    try {
        constraints = JSON.parse(textarea.value);
    } catch (e) {
        console.error(e);
        setError('ERROR parsing constraints: ' + e.message + ', using default constraints');
        constraints = default_constraints;
    }
    console.log(JSON.stringify(constraints));

    // Add local stream
    if (navigator.mediaDevices.getUserMedia) {
        return navigator.mediaDevices.getUserMedia(constraints);
    } else {
        errorUserMediaHandler();
    }
}

function websocketServerConnect() {
    connect_attempts++;
    if (connect_attempts > 3) {
        setError("Too many connection attempts, aborting. Refresh page to try again");
        return;
    }
    // Clear errors in the status span
    var span = document.getElementById("status");
    span.classList.remove('error');
    span.textContent = '';
    // Populate constraints
    /* var textarea = document.getElementById('constraints');
    if (textarea.value == '')
        textarea.value = JSON.stringify(default_constraints);*/
    // Fetch the peer id to use
    our_id = our_id || getOurId();
    ws_port = ws_port || '8443';
    if (window.location.protocol.startsWith ("file")) {
        ws_server = ws_server || "127.0.0.1";
    } else if (window.location.protocol.startsWith ("http")) {
        ws_server = ws_server || window.location.hostname;
    } else {
        throw new Error ("Don't know how to connect to the signalling server with uri" + window.location);
    }
    var ws_url = 'ws://' + ws_server + ':' + ws_port
    setStatus("Connecting to server " + ws_url);
    ws_conn = new WebSocket(ws_url);
    /* When connected, immediately register with the server */
    ws_conn.addEventListener('open', (event) => {
        document.getElementById("peer-id").textContent = our_id;
        // ws_conn.send('HELLO ' + peer_id);
		var jsonObj = {
		    "direction": "cs",
		    "seq"      : seq++,
		    "from"     : our_id,
		    "role"     : role,
		    "content"  : {
		        "type" : "register",
		        "uid"  : our_id
		    }
		};
		ws_conn.sendString(JSON.stringify(jsonObj));
        setStatus("Registering with server");
    });
    ws_conn.addEventListener('error', onServerError);
    ws_conn.addEventListener('message', onServerMessage);
    ws_conn.addEventListener('close', onServerClose);
	
	ws_conn.sendString = function(str) {
	    console.log("Sending string: " + str);
	    ws_conn.send(str);
	}
}

/* function getStats(peer) {
    myGetStats(peer, function (results) {
        for (var i = 0; i < results.length; ++i) {
            var res = results[i];
			if( res.type == "ssrc" ) {
				console.log(res);
			} 
			console.log(res);
        }

        setTimeout(function () {
            getStats(peer);
        }, 1000);
    });
} */

function onTrackWrapper(channel) {
	return function onTrack(event) {
		console.log(event);
        for( s in event.streams ) {
            try {
                var videoElement = getVideoElement(channel);
                videoElement.srcObject = event.streams[0];
            } catch (e) {
                console.error(e);
                handleIncomingError('Stream with unknown tracks added, resetting');
            }
        }
	}
}

function onRemoteStreamAddedWrapper(channel) {
    return function onRemoteStreamAdded(event) {
        console.log(event);

        videoTracks = event.stream.getVideoTracks();
        audioTracks = event.stream.getAudioTracks();

        if (videoTracks.length > 0) {
            console.log('Incoming stream: ' + videoTracks.length + ' video tracks and ' + audioTracks.length + ' audio tracks');
            var videoElement = getVideoElement(channel);
            videoElement.srcObject = event.stream;
        } else {
            handleIncomingError('Stream with unknown tracks added, resetting');
        }
    }
}


function errorUserMediaHandler() {
    setError("Browser doesn't support getUserMedia!");
}

const handleDataChannelOpen = (event) =>{
    console.log("dataChannel.OnOpen", event);
};

const handleDataChannelMessageReceived = (event) =>{
    console.log("dataChannel.OnMessage:", event, event.data.type);

    setStatus("Received data channel message");
    if (typeof event.data === 'string' || event.data instanceof String) {
        console.log('Incoming string message: ' + event.data);
        textarea = document.getElementById("text")
        textarea.value = textarea.value + '\n' + event.data
    } else {
        console.log('Incoming data message');
    }
    send_channel.send("Hi! (from browser)");
};

const handleDataChannelError = (error) =>{
    console.log("dataChannel.OnError:", error);
};

const handleDataChannelClose = (event) =>{
    console.log("dataChannel.OnClose", event);
};

function onDataChannel(event) {
    setStatus("Data channel created");
    let receiveChannel = event.channel;
    receiveChannel.onopen = handleDataChannelOpen;
    receiveChannel.onmessage = handleDataChannelMessageReceived;
    receiveChannel.onerror = handleDataChannelError;
    receiveChannel.onclose = handleDataChannelClose;
}

function createCall(peer_connections) {
    // Reset connection attempts because we connected successfully
    connect_attempts = 0;

    console.log('Creating RTCPeerConnection');
	
	for( i in peer_connections ) {
		peer_connections[i] = new RTCPeerConnection(rtc_configuration);
		// send_channel = peer_connections[i].createDataChannel('label', null);
		// send_channel.onopen = handleDataChannelOpen;
		// send_channel.onmessage = handleDataChannelMessageReceived;
		// send_channel.onerror = handleDataChannelError;
		// send_channel.onclose = handleDataChannelClose;
		peer_connections[i].ondatachannel = onDataChannel;
		// peer_connections[i].onaddstream = onRemoteStreamAddedWrapper(i);
        peer_connections[i].ontrack = onTrackWrapper(i);
		/* Send our video/audio to the other peer */
		local_stream_promise[i] = getLocalStream().then((stream) => {
		    console.log('Adding local stream');
		    peer_connections[i].addStream(stream);
		    return stream;
		}).catch(setError);
		
		peer_connections[i].onicecandidate = handleIceCandidateAnswerWrapper(i);
		
	}
    setStatus("Created peer connection for call, waiting for SDP");
}

 function handleIceCandidateAnswerWrapper(channel) {
	 return function handleIceCandidate(event) {
		console.log('onicecandidate triggered ', channel);
		var ice = "";
		var isEmptyIce = true;
		if (event.candidate) {
			ice = event.candidate;
			isEmptyIce = false;
		}
		if (!isEmptyIce) {
			sendCandidateCount++;
			var jsonObj = {
				"direction": "p2p",
				"seq": seq++,
				"from": our_id,
				"to": peer_id,
				"role": role,
				"content": {
					"type": "ice",
					"index": channel,
					"candidateLine": ice,
				}
			};
			ws_conn.sendString(JSON.stringify(jsonObj));
			console.log("count of sent candidate: " + sendCandidateCount);
		} else {
			console.log("all local candidates sent. count of total sent candidate: " + sendCandidateCount);
		}
	}
}

function switchVideoMode(modeRadio) {
    currentValue = modeRadio.value;
	if (currentValue == "auto" ) {
		flag = true;
	} else if (currentValue == "manual" ) {
		flag = false;
	} 
	document.getElementById('right_camera').disabled = flag;
	document.getElementById('front_camera').disabled = flag;
	document.getElementById('left_camera').disabled = flag;
	document.getElementById('back_camera').disabled = flag;
}

// ICE candidate received from peer, add it to the peer connection
/* function onIncomingICE(ice) {
    var candidate = new RTCIceCandidate(ice);
    peer_connection.addIceCandidate(candidate).catch(setError);
} */

/* var createOffer = function(stream) {
    console.log("create offer");
    //setupPeerConnection();
    //dataChannel = peerConnection.createDataChannel(dataChannelLabel);
    //setupDataChannel(dataChannel);
    peer_connection.createOffer().then(function(offer) {
            return peer_connection.setLocalDescription(offer);
        })
        .then(function() {
            var jsonObj = {
                "direction": "p2p",
                "seq": seq++,
                "from": our_id,
                "to": peer_id,
                "role": role,
                "content": peer_connection.localDescription
            };
						jsonObj.content.index = stream;
            ws_conn.sendString(JSON.stringify(jsonObj));
        })
        .catch(function(reason) {
            // An error occurred, so handle the failure to connect
            console.log("RTC Error", reason);
        });
};

 */

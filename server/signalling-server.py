#!/usr/bin/env python3
#
# Example 1-1 call signalling server
#
# Copyright (C) 2017 Centricular Ltd.
#
#  Author: Nirbheek Chauhan <nirbheek@centricular.com>
#

import os
import sys
import ssl
import logging
import asyncio
import websockets
import argparse
import json

from concurrent.futures._base import TimeoutError

parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
parser.add_argument('--addr', default='0.0.0.0', help='Address to listen on')
parser.add_argument('--port', default=8443, type=int, help='Port to listen on')
parser.add_argument('--keepalive-timeout', dest='keepalive_timeout', default=30, type=int, help='Timeout for keepalive (in seconds)')
parser.add_argument('--cert-path', default=os.path.dirname(__file__))
parser.add_argument('--disable-ssl', default=False, help='Disable ssl', action='store_true')

options = parser.parse_args(sys.argv[1:])

ADDR_PORT = (options.addr, options.port)
KEEPALIVE_TIMEOUT = options.keepalive_timeout

############### Global data ###############

# Format: {uid: (Peer WebSocketServerProtocol,
#                remote_address,
#                <'session'|room_id|None>)}
peers_car_video = dict()
peers_car_data = dict()
peers_controller_video = dict()
peers_controller_data = dict()
# Format: {caller_uid: callee_uid,
#          callee_uid: caller_uid}
# Bidirectional mapping between the two peers
sessions_video = dict()
sessions_data = dict()

############### Helper functions ###############

async def recv_msg_ping(ws, raddr):
    '''
    Wait for a message forever, and send a regular ping to prevent bad routers
    from closing the connection.
    '''
    msg = None
    while msg is None:
        try:
            msg = await asyncio.wait_for(ws.recv(), KEEPALIVE_TIMEOUT)
        except TimeoutError:
            print('Sending keepalive ping to {!r} in recv'.format(raddr))
            await ws.ping()
    return msg

async def disconnect(ws, uid):
    '''
    Remove @peer_id from the list of sessions and close our connection to it.
    This informs the peer that the session and all calls have ended, and it
    must reconnect.
    '''
    print("run function: disconnect............")
    global sessions_data, sessions_video
    if uid in sessions_data:
        del sessions_data[uid]
    elif uid in sessions_video:
        del sessions_video[uid]
    # Close connection
    if ws and ws.open:
        # Don't care about errors
        asyncio.ensure_future(ws.close(reason='hangup'))

async def cleanup_session(msgJson):
    print("run function: cleanup_session............")
    uid = msgJson["from"]
    classes = msgJson["role"]
    peerId = None
    peer_status = None
    print("id {} .....1".format(uid))

    if classes == 'vehicle.video':
        if uid in sessions_video:
            peerId = sessions_video[uid]
            del sessions_video[uid]
            print("Cleaned up {} video session".format(uid))
            peers_car_video[uid][2] = peer_status
            print("video peer {} state reseted".format(uid))
            if peerId in sessions_video:
                del sessions_video[peerId]
                print("Also cleaned up {} video session".format(peerId))
                if peerId in peers_controller_video:
                    peers_controller_video[peerId][2] = peer_status
                    print("video peer {} state reseted".format(peerId))
    elif classes == 'vehicle.control':
        if uid in sessions_data:
            peerId = sessions_data[uid]
            del sessions_data[uid]
            print("Cleaned up {} data session".format(uid))
            peers_car_data[uid][2] = peer_status
            print("data peer {} state reseted".format(uid))
            if peerId in sessions_data:
                del sessions_data[peerId]
                print("Also cleaned up {} data session".format(peerId))
                if peerId in peers_controller_data:
                    peers_controller_data[peerId][2] = peer_status
                    print("data peer {} state reseted".format(peerId))
    elif classes == 'controller.video':
        if uid in sessions_video:
            peerId = sessions_video[uid]
            del sessions_video[uid]
            print("Cleaned up {} video session".format(uid))
            peers_controller_video[uid][2] = peer_status
            print("video peer {} state reseted".format(uid))
            if peerId in sessions_video:
                del sessions_video[peerId]
                print("Also cleaned up {} video session".format(peerId))
                if peerId in peers_car_video:
                    peers_car_video[peerId][2] = peer_status
                    print("video peer {} state reseted".format(peerId))
    elif classes == 'controller.control':
        if uid in sessions_data:
            peerId = sessions_data[uid]
            del sessions_data[uid]
            print("Cleaned up {} data session".format(uid))
            peers_controller_data[uid][2] = peer_status
            print("data peer {} state reseted".format(uid))
            if peerId in sessions_data:
                del sessions_data[peerId]
                print("Also cleaned up {} data session".format(peerId))
                if peerId in peers_car_data:
                    peers_car_data[peerId][2] = peer_status
                    print("data peer {} state reseted".format(peerId)) 

async def remove_peer(ws, msgJson):
    await cleanup_session(msgJson)
    print("run function: remove_peer..........")
    uid     = msgJson["from"];
    classes = msgJson["role"];
    print("id {} .....2".format(uid))

    if classes == 'vehicle.control': 
        if uid in peers_car_data:
            #wso, raddr, status = peers_car_data[uid]
            del peers_car_data[uid]
    elif classes == 'vehicle.video':
        if uid in peers_car_video:
            del peers_car_video[uid]
    elif classes == 'controller.control': 
        if uid in peers_controller_data:
            del peers_controller_data[uid]
            if uid in peers_controller_video:
                wso, raddr, status = peers_controller_video[uid]
                await wso.close()
                print("Disconnect from controller_video peer {!r} at {!r}".format(uid, raddr))
                del peers_controller_video[uid]
    elif classes == 'controller.video':
        if uid in peers_controller_video:
            del peers_controller_video[uid]
            if uid in peers_controller_data:
                wso, raddr, status = peers_controller_data[uid]
                await wso.close()
                print("Disconnect from controller_data peer {!r} at {!r}".format(uid, raddr))
                del peers_controller_data[uid]
    await ws.close()
    print("Disconnected from peer {!r} at {!r}".format(uid, ws.remote_address))

############### Handler functions ###############
async def p2p_handler(ws, msgJson):
    global sessions_video, sessions_data
    raddr = ws.remote_address
    peer_status = None
    wso = None
    seq     = msgJson["seq"];
    uid     = msgJson["from"];
    classes = msgJson["role"];
    peerId  = msgJson["to"];

    # Update current status
    if classes == 'vehicle.control':
        peer_status = peers_car_data[uid][2]
    elif classes == 'vehicle.video':
        peer_status = peers_car_video[uid][2]
    elif classes == 'controller.control':
        peer_status = peers_controller_data[uid][2]
    elif classes == 'controller.video':
        peer_status = peers_controller_video[uid][2]
    else:
        sendMsg = json.dumps({'direction': 'sc', 'seq' : seq, 'content': { 'type': 'p2p_response', 'result': 'p2p_failed' }})
        await ws.send(sendMsg)
        await ws.close(code=1002, reason='invalid classes')
        raise Exception("Invalid classes from {!r}".format(raddr))

    # peers_status is not None 
    if peer_status is not None:
        # forwarding messages to peerId
        if peer_status == 'session':
            # if classes == 'controller.control':
            #     wso, oaddr, status = peers_car_data[peerId]
            # elif classes == 'controller.video':
            #     wso, oaddr, status = peers_car_video[peerId]
            # elif classes == 'vehicle.control':
            #     wso, oaddr, status = peers_controller_data[peerId]
            # elif classes == 'vehicle.video':
            #     wso, oaddr, status = peers_controller_video[peerId]
            if classes == 'vehicle.video' or classes == 'controller.video':
                other_id = sessions_video[uid]
            else:
                other_id = sessions_data[uid]
            if classes == 'controller.control':
                wso, oaddr, status = peers_car_data[other_id]
            elif classes == 'controller.video':
                wso, oaddr, status = peers_car_video[other_id]
            elif classes == 'vehicle.control':
                wso, oaddr, status = peers_controller_data[other_id]
            else:
                wso, oaddr, status = peers_controller_video[other_id]
            assert(status == 'session')
            sendMsg = json.dumps(msgJson)
            # print("{} -> {}: {}".format(uid, peerId, sendMsg))
            await wso.send(sendMsg)
        else:
             raise AssertionError('Unknown peer status {!r}'.format(peer_status))

    else:
        # failed to create seesion for uid, then send message back to uid
        print("failed to forwarding p2p messages")
        sendMsg = json.dumps({'direction': 'sc', 'seq' : seq, 'content': { 'type': 'p2p_response', 'result': 'p2p_failed', 'reason': 'no session created for you, peer_status is None' }})
        await ws.send(sendMsg)
        await ws.close(code=1002, reason='invalid session')
        raise Exception("Invalid session from {!r}".format(raddr))


async def cs_handler(ws, msgJson):
    global peers_car_data, peers_car_video, peers_controller_data, peers_controller_video, sessions_video, sessions_data
    seq     = msgJson["seq"];
    uid     = msgJson["from"];
    classes = msgJson["role"];
    msgType = msgJson["content"]["type"];
    
    raddr = ws.remote_address
    peer_status = None
    callee_id = None
    wsc = None
    register_flag = None

    if msgType == 'register':
        if classes == 'controller.control':
            if not uid or uid in peers_controller_data  :
                register_flag = 'false'
            else:
                register_flag = 'true'
                peers_controller_data[uid] = [ws, raddr, peer_status]

        elif classes == 'controller.video':
            if not uid or uid in peers_controller_video :
                register_flag = 'false'      
            else:
                peers_controller_video[uid] = [ws, raddr, peer_status]
                register_flag = 'true'

        elif classes == 'vehicle.control':
            if not uid or uid in peers_car_data :
                register_flag = 'false'
            else:
                peers_car_data[uid] = [ws, raddr, peer_status]
                register_flag = 'true'

        elif classes == 'vehicle.video':
            if not uid or uid in peers_car_video:
                register_flag = 'false'
            else:
                peers_car_video[uid] = [ws, raddr, peer_status]
                register_flag = 'true'
        else:
            sendMsg = json.dumps({'direction': 'sc', 'seq' : seq, 'content': { 'type': 'register_response', 'result': 'failed', 'reason': 'unknown role' }})
            await ws.send(sendMsg)
            await ws.close(code=1002, reason='invalid classes')
            raise Exception("Invalid classes from {!r}".format(raddr))

        if register_flag == 'false':
            sendMsg = json.dumps({'direction': 'sc', 'seq' : seq, 'content': { 'type': 'register_response', 'result': 'failed', 'reason': 'not caller_id or caller_id has already registered' }})
            await ws.send(sendMsg)
            await ws.close(code=1002, reason='register failed')
            raise Exception("Register failed from {!r}".format(raddr))
        elif register_flag == 'true':
            sendMsg = json.dumps({'direction': 'sc', 'seq' : seq, 'content': { 'type': 'register_response', 'result': 'success' }})
            await ws.send(sendMsg)
        # else:
            # sendMsg = json.dumps({'direction': 'sc', 'seq' : seq, 'content': { 'type': 'register_response', 'result': 'register_failed', 'reason': 'unknown role' }})
            # await ws.send(sendMsg)

    elif msgType == 'call':
        peerId  = msgJson["content"]["peer"];
    # =================call for sessions===================
        # Update current status
        if classes == 'vehicle.control':
            peer_status = peers_car_data[uid][2]
        elif classes == 'vehicle.video':
            peer_status = peers_car_video[uid][2]
        elif classes == 'controller.control':
            peer_status = peers_controller_data[uid][2]
        elif classes == 'controller.video':
            peer_status = peers_controller_video[uid][2]
        else:
            sendMsg = json.dumps({'direction': 'sc', 'seq' : seq, 'content': { 'type': 'call_response', 'result': 'failed', 'reason': 'unknow role'}})
            await ws.send(sendMsg)
            await ws.close(code=1002, reason='invalid classes')
            raise Exception("Invalid classes from {!r}".format(raddr))
        #############################################################################################
        # Requested a session
        # modified by liyujia, 20180927
        if peer_status == None:
            print ("Now, create session for {!r}".format(uid))
            # peerId is None, find one available peerId in peers list and create session
            if peerId == None: 
                print('peerId is none')  
                if classes == 'vehicle.video':
                    if peers_controller_video is not None:
                        for (_id, _vlaue) in peers_controller_video.items():
                            if peers_controller_video[_id][2] != 'session':
                                callee_id = _id
                                wsc = peers_controller_video[callee_id][0]    
                                # create session
                                peers_car_video[uid][2] = peer_status = 'session'
                                sessions_video[uid] = callee_id
                                peers_controller_video[callee_id][2] = 'session'
                                sessions_video[callee_id] = uid 
                                break
                
                elif classes == 'vehicle.control':
                    if peers_controller_data is not None:
                        for (_id, _vlaue) in peers_controller_data.items():
                            if peers_controller_data[_id][2] != 'session':
                                callee_id = _id
                                wsc = peers_controller_data[callee_id][0]
                                peers_car_data[uid][2] = peer_status = 'session'
                                sessions_data[uid] = callee_id
                                peers_controller_data[callee_id][2] = 'session'
                                sessions_data[callee_id] = uid
                                break
                elif classes == 'controller.video':
                    if peers_car_video is not None:
                        for (_id, _vlaue) in peers_car_video.items():
                            if peers_car_video[_id][2] != 'session':
                                callee_id = _id
                                wsc = peers_car_video[callee_id][0]
                                peers_controller_video[uid][2] = peer_status = 'session'
                                sessions_video[uid] = callee_id
                                peers_car_video[callee_id][2] = 'session'
                                sessions_video[callee_id] = uid
                                sendMsg = json.dumps({'direction': 'sc', 'seq' : seq, 'content': { 'type': 'call', 'peerId': uid }})
                                await wsc.send(sendMsg)                      
                                break
                elif classes == 'controller.control':
                    if peers_car_data is not None:
                        print('peers_car_data is not none')  
                        for (_id, _vlaue) in peers_car_data.items():
                            if peers_car_data[_id][2] != 'session':
                                print('peer id == {!r}'.format(_id))
                                callee_id = _id
                                wsc = peers_car_data[callee_id][0]
                                peers_controller_data[uid][2] = peer_status = 'session'
                                sessions_data[uid] = callee_id
                                peers_car_data[callee_id][2] = 'session'
                                sessions_data[callee_id] = uid
                                break
                if callee_id is not None:  # Send back to Initiator
                    sendMsg = json.dumps({'direction': 'sc', 'seq' : seq, 'content': { 'type': 'call_response', 'result': 'success', 'peerId': callee_id }}) 
                    await ws.send(sendMsg)
                else:
                    sendMsg = json.dumps({'direction': 'sc', 'seq' : seq, 'content': { 'type': 'call_response', 'result': 'failed', 'reason': 'peer is not online' }})
                    await ws.send(sendMsg)
                    await ws.close(code=1002, reason='no avalible peer')
                    raise Exception("Call failed from {!r}".format(raddr))

            else:  # peerId is not None
                print('peerId is not none')  
                if classes == 'vehicle.video':
                # check if the peerId has registered and check if the peerId is already in session list
                    if peerId in peers_controller_video and peers_controller_video[peerId][2] != 'session':
                        #create session for uid with the specific peerId
                        peers_car_video[uid][2] = peer_status = 'session'
                        callee_id = peerId
                        wsc = peers_controller_video[callee_id][0]
                        sessions_video[uid] = callee_id
                        peers_controller_video[callee_id][2] = 'session'
                        sessions_video[callee_id] = uid
                elif classes == 'vehicle.control':
                    if peerId in peers_controller_data and peers_controller_data[peerId][2] != 'session':
                        peers_car_data[uid][2] = peer_status = 'session'
                        callee_id = peerId
                        wsc = peers_controller_data[callee_id][0]
                        sessions_data[uid] = callee_id
                        peers_controller_data[callee_id][2] = 'session'
                        sessions_data[callee_id] = uid
                elif classes == 'controller.video':
                    if peerId in peers_car_video and peers_car_video[peerId][2] != 'session':
                        peers_controller_video[uid][2] = peer_status = 'session'
                        callee_id = peerId
                        wsc = peers_car_video[callee_id][0]
                        sessions_video[uid] = callee_id
                        peers_car_video[callee_id][2] = 'session'
                        sessions_video[callee_id] = uid
                        sendMsg = json.dumps({'direction': 'sc', 'seq' : seq, 'to' : peerId, 'content': { 'type': 'call', 'peerId': uid }})
                        await wsc.send(sendMsg)
                elif classes == 'controller.control':
                    if peerId in peers_car_data and peers_car_data[peerId][2] != 'session':
                        peers_controller_data[uid][2] = peer_status = 'session'
                        callee_id = peerId
                        wsc = peers_car_data[callee_id][0]
                        sessions_data[uid] = callee_id
                        peers_car_data[callee_id][2] = 'session'
                        sessions_data[callee_id] = uid
                if callee_id is not None:
                    sendMsg = json.dumps({'direction': 'sc', 'seq' : seq, 'to': callee_id,  
                                          'content': { 'type': 'call_response', 'result': 'success' }})
                    await ws.send(sendMsg)
               
                else:
                    sendMsg = json.dumps({'direction': 'sc', 'seq' : seq, 'to': callee_id, 
                                          'content': { 'type': 'call_response', 'result': 'failed', 'reason': 'peer is not online' }})
                    await ws.send(sendMsg)
                    # await ws.close(code=1002, reason='wrong peer id')
                    # raise Exception("Call failed from {!r}, wrong peer id".format(raddr))
    else:
        sendMsg = json.dumps({'direction': 'sc', 'seq' : seq, 'content': { 'type': 'response', 'result': 'unknown msg type' }})
        await ws.send(sendMsg)
        await ws.close(code=1002, reason='invalid msg type')
        raise Exception("Invalid msg type from {!r}".format(raddr))

async def handler(ws, path):
    '''
    All incoming messages are handled here. @path is unused.
    '''
    raddr = ws.remote_address
    print("Connected to {!r}".format(raddr))
    #    'register': reg_handler(ws, msgJson),
    #    'p2p'     : p2p_handler(ws, msgJson)
    #}[msgType]()
    msgJson = None
    try:
        while(True):
            msg = await recv_msg_ping(ws, raddr)
            # msg = await ws.recv()
            print('{!r}'.format(msg)) 
            msgJson = json.loads(msg)
            msgType = msgJson["direction"]

            if msgType == 'cs':
                await cs_handler(ws, msgJson)
            elif msgType == 'p2p':
                await p2p_handler(ws, msgJson)
            #{
            #    'register': await reg_handler(ws, msgJson),
            #    'p2p'     : await p2p_handler(ws, msgJson)
            #}[msgType]()
    except json.decoder.JSONDecodeError:
        print("Failed to Decode Message in JSON format")
    except websockets.ConnectionClosed:
        print("Connection to peer {!r} closed, exiting handler".format(raddr))
    finally:
        print('msgJson : {!r}'.format(msgJson))
        await remove_peer(ws, msgJson)

sslctx = None
if not options.disable_ssl:
    # Create an SSL context to be used by the websocket server
    certpath = options.cert_path
    print('Using TLS with keys in {!r}'.format(certpath))
    if 'letsencrypt' in certpath:
        chain_pem = os.path.join(certpath, 'fullchain.pem')
        key_pem = os.path.join(certpath, 'privkey.pem')
    else:
        chain_pem = os.path.join(certpath, 'cert.pem')
        key_pem = os.path.join(certpath, 'key.pem')

    sslctx = ssl.create_default_context()
    try:
        sslctx.load_cert_chain(chain_pem, keyfile=key_pem)
    except FileNotFoundError:
        print("Certificates not found, did you run generate_cert.sh?")
        sys.exit(1)
    # FIXME
    sslctx.check_hostname = False
    sslctx.verify_mode = ssl.CERT_NONE

print("Listening on http://{}:{}".format(*ADDR_PORT))
# Websocket server
wsd = websockets.serve(handler, *ADDR_PORT, ssl=sslctx,
                       # Maximum number of messages that websockets will pop
                       # off the asyncio and OS buffers per connection. See:
                       # https://websockets.readthedocs.io/en/stable/api.html#websockets.protocol.WebSocketCommonProtocol
                       max_queue=16)

logger = logging.getLogger('websockets.server')

logger.setLevel(logging.ERROR)
logger.addHandler(logging.StreamHandler())

asyncio.get_event_loop().run_until_complete(wsd)
asyncio.get_event_loop().run_forever()

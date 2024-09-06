#!/usr/bin/python3

import asyncio
import websockets
import json
import msgpack


""" Test the WebSocketsServer module for DUECA

Possible url's offered by the server:

* /configuration
  overview of the configured channels

* /read/<name>?entry=n
  read JSON with a single time tick and single set of data

* /current/<name>?entry=n
  for each message, get a JSON with current data

* /info/<name>
  get a cookie for channel info changes

* /write/<name>
  write entry

* /write-and-read/<name>
  write and read server configuration

"""

# read and decode configuration data, print it
async def configdata(decoder, port):
    url = f'ws://127.0.0.1:{port}/configuration'
    async with websockets.connect(url) as wsock:
        print("connection to", url)

        try:
            print("waiting for data", url)
            data = await wsock.recv()
            info = decoder.loads(data)
            print ("info", info)
            wsock.close()

        except Exception as e:
            print(f"Configuration, exception {e}")

async def pumparound(coder, port, inpoint, outpoint, label):

    urlin = f'ws://127.0.0.1:{port}/read/{inpoint}?entry=0'
    urlout = f'ws://127.0.0.1:{port}/write/{outpoint}'
    firstmsg = True

    try:

        async with websockets.connect(urlin), websockets.connect(urlout) as wsockin, wsockout:
            print("Established read and write connection to", urlin, urlout)

            while True:

                msg = await wsockin.recv()
                data = coder.loads(msg)

                if firstmsg:
                    conf = dict(label=label, ctiming=True, event=False)
                    print("Configuring write set-up", conf)
                    res = await wsockout.send(coder.dumps(conf))

                print("Replying message", data)
                res = await wsockout.send(coder.dumps(data))
    except Exception as e:
        print(f"Write {urlin} to {urlout}, exception {e}")

#! /usr/bin/env python
# -*- coding: utf-8 -*-
# vim:fenc=utf-8
#
# Copyright (c) 2024 bookerbai <bookerbai@tencent.com>
# Create time: 2024-07-29 15:14
# Distributed under terms of the Tencent license.

"""
一个ws client连接实例
"""
import asyncio
import websockets

async def send_heartbeat(websocket, message):
    while True:
        try:
            await websocket.send(message)
            response = await websocket.recv()
            print(f"Received: {response}")
        except websockets.exceptions.ConnectionClosedOK:
            print("Connection closed by server (going away). Attempting to reconnect...")
            break
        except websockets.exceptions.ConnectionClosedError as e:
            print(f"Connection closed with error: {e}. Attempting to reconnect...")
            break
        await asyncio.sleep(1)

async def main():
    uri = "ws://192.168.33.206:80"
    while True:
        try:
            async with websockets.connect(uri) as websocket:
                await send_heartbeat(websocket, "1;-10;10")
        except Exception as e:
            print(f"An error occurred: {e}")
        print("Reconnecting in 5 seconds...")
        await asyncio.sleep(1)

if __name__ == "__main__":
    asyncio.run(main())

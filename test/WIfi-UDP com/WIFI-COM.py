import socket
import struct
import cv2 as cv

ESP32_IP = "10.106.8.131"
UDP_PORT = 8888

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)


def calcCRC(data):
    crc = 0

    for byte in data:
        crc ^= byte

    return crc


def shipPack(cmd, isSweeping, speedDelay):
    data = struct.pack("<BBB", cmd, isSweeping, speedDelay)

    crc = calcCRC(data)

    packet = data + bytes([crc])

    sock.sendto(packet, (ESP32_IP, UDP_PORT))


def onTrackbarChange(value):
    cmd = cv.getTrackbarPos("cmd", "packets")
    isSweeping = cv.getTrackbarPos("isSweeping", "packets")
    speedDelay = cv.getTrackbarPos("speedDelay", "packets")

    shipPack(cmd, isSweeping, speedDelay)


cv.namedWindow("packets")

cv.createTrackbar("cmd", "packets", 1, 2, onTrackbarChange)
cv.createTrackbar("isSweeping", "packets", 0, 1, onTrackbarChange)
cv.createTrackbar("speedDelay", "packets", 0, 100, onTrackbarChange)


while True:
    if cv.waitKey(1) == 27:
        break

cv.destroyAllWindows()
import paho.mqtt.client as mqtt
import requests

MQTT_SERVER = "192.168.137.41"
MQTT_PATH = "#"
URI = 'http://192.168.137.41/final_project/api.php'
 
# The callback when conected.
def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc)) 
    client.subscribe(MQTT_PATH)
 
# Callback when message received
def on_message(client, userdata, msg):
    # print("\t\t*"+msg.topic+"\t\t*\t\t\t "+str(msg.payload))
    # print(msg.payload.decode('utf-8'))
     
    NodeID = msg.payload.decode('utf-8').split(",")[0]
    Value = msg.payload.decode('utf-8').split(",")[1]

    if msg.topic == "iotfinal/temp1":
        response = requests.post(URI, json={"NodeID":f"{NodeID}","TypeID":"1", "Value":Value})
        print(f"Node {NodeID} positng temp Value {Value}")
        print("temp status: ", response.status_code)
    elif msg.topic == "iotfinal/hum1":
        response = requests.post(URI, json={"NodeID":f"{NodeID}","TypeID":"3", "Value":Value})
        print(f"Node {NodeID} positng humidity Value {Value}")
        print("hum status: ", response.status_code)
    elif msg.topic == "iotfinal/light1":
        response = requests.post(URI, json={"NodeID":f"{NodeID}","TypeID":"2", "Value":Value})
        print(f"Node {NodeID} positng light Value {Value}")
        print("light status: ", response.status_code)
    else:
        # Handle messages from other topics
        pass


client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION1)
client.on_connect = on_connect
client.on_message = on_message
 
client.connect(MQTT_SERVER, 1883, 60)

print("waiting for messages")
client.loop_forever()

#client.loop_start() #start the loop
#time.sleep(10) # wait
#client.loop_stop() #stop the loop
 
# Blocking call-  processes messages client.loop_forever()
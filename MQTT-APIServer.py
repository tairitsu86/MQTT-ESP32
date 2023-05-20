from flask import Flask, request, jsonify
import requests
import paho.mqtt.client as mqtt


topic = 'esp32/Control'
state = 'OFF'  # Define the 'state' variable
tdata = {
    'Humidity':'',
    'Temperature':''
}

# 連線設定
# 初始化地端程式
client = mqtt.Client()

# 當地端程式連線伺服器得到回應時，要做的動作
def on_connect(client, userdata, flags, rc):
    print("Connected with result code " + str(rc))
    client.subscribe("esp32/Humidity")
    client.subscribe("esp32/Temperature")
    client.subscribe("esp32/State")

# 當接收到從伺服器發送的訊息時要進行的動作
def on_message(client, userdata, msg):
    # Decode payload to string using UTF-8 encoding
    global tdata
    global state
    # print(msg.topic,msg.payload.decode('utf-8'))
    if msg.topic=='esp32/Humidity' :
        tdata['Humidity'] = msg.payload.decode('utf-8')
    elif msg.topic=='esp32/Temperature' :
        tdata['Temperature'] = msg.payload.decode('utf-8')
    elif msg.topic=='esp32/State' :
        state = msg.payload.decode('utf-8')


# 設定連線的動作
client.on_connect = on_connect

# 設定接收訊息的動作
client.on_message = on_message

# 設定連線資訊(IP, Port, 連線時間)
client.connect("140.136.151.74", 1883)
# 開始連線，執行設定的動作和處理重新連線問題
client.loop_start()



app = Flask(__name__)

@app.route('/on')
def turn_on():
    client.publish(topic, 'ON')
    return 

@app.route('/off')
def turn_off():
    client.publish(topic, 'OFF')
    return 

@app.route('/state')
def get_state():
    return jsonify({"State": state})

@app.route('/data')
def get_data():
    return jsonify({"data":f"Temperature: {tdata['Temperature']},Humidity: {tdata['Humidity']}"})  # Placeholder data
@app.route('/auto')
def turn_auto():
    client.publish(topic, 'AUTO')

@app.route('/iotWebhook',methods=['POST'])
def webhookHandler():
    data = request.get_json()
    message = data['message']
    result = 'Error'
    if message == 'ON':
        turn_on()
        result = 'The fan has been turned ON.'
    elif message == 'OFF':
        turn_off()
        result = 'The fan has been turned OFF.'
    elif message == 'AUTO':
        turn_auto()
        result = 'The fan has been turned to AUTO mode.'
    elif message == 'STATE':
        result = f'The fan is {state}.'
    elif message == 'TH':
        result = f"Temperature: {tdata['Temperature']}°C,Humidity: {tdata['Humidity']}%."
        
    replyEvent = {
        'reportMessage':f'{result}'
    }
    return replyEvent
     

if __name__ == '__main__':
    app.run(debug=True,host='140.136.151.74', port=8080)

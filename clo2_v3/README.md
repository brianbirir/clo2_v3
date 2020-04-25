# **clo2_v3**

## **Topics**
These are the new device topics

```project/clo2/device/<device_id>/command```

and 

```project/clo2/device/<device_id>/device_data```

Where ```*/command``` is the topic to send commands to the device. 
while ```*/device_data``` is the topic where the device will send its data.

for example
chocolate_charlie will use the following topics respectively

```project/clo2/device/e00fce683f0ea854ec94ca8d/command```

```project/clo2/device/e00fce683f0ea854ec94ca8d/device_data```

## **MQTT Commands**
The MQTT commands may be sent to the device as follows.  
**{"CC":1 ,"TOGGLE_SYSTEM":true ,"RESET":true ,"PREP_TIME":10 ,"CHLORINATION_TIME":3 , "CC":1}**

Where 

**TOGGLE_SYSTEM** will start the device.  
**RESET** will reset the system back to defaults.  
**PREP_TIME** is the duration for preperation period in mins.  
**CHLORINATION_TIME** is the duration for chlorination period in mins.  
**CC** is a message counter. Its value should always increase with each message successfully sent to the device.  
```*If a repeated command counter number is sent, or if the number is less than the previous number, the message will be ignored by the device.*```

To know that a message was successfully sent, the device will return a message in the following format.

**{"PCID":1 ,"TOGGLE_SYSTEM":true ,"RESET":true ,"PREP_TIME":10 ,"CHLORINATION_TIME":3 , "PCID":1}**

where  
**PCID** value should match the last processed command.

**NOTE** all the commands in te json string are optional except for the CC at the beginning and at the end.  
you may send individual commands like so  
**{"CC":21 ,"TOGGLE_SYSTEM":true , "CC":21}**

and expect a response as follows

**{"CC":21 ,"TOGGLE_SYSTEM":true , "CC":21}**

## **MQTT device data**
During the normal running of the device, the device will send notifications to MQTT containing relevant system information such as the state it's currently in.  
The notification messages will be in the following formats.  

Whenever the system goes from idle to running or the opposite, the following will be sent  
**{"STATUS":12,"TRIGGER_SOURCE":TRIG_MQTT,"SYSTEM_STATE":false,"STATUS":12 }**

Whenever the system changes states within its running process  
**{"STATUS":11,"SYSTEM_STATE":false,"SYSTEM_OP_STATE":IDLE,"PREP_TIME":10000,"PREP_TIME":10000,"STATUS":11 }**

Where 

**TRIGGER_SOURCE** tells whether the device was triggered by a button or over MQTT  
**STATUS** is an increasing number denoting how manu status messages a device has sent since the last time it reset  
**SYSTEM_STATE** shows whether the system is ON or OFF  
**SYSTEM_OP_STATE** shows in which stage of the process the device is in  

## ***You may use [MQTT fx](http://www.jensd.de/apps/mqttfx/1.7.0/) to test the device using these commands**
 

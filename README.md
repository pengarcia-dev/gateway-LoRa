This script gets the message that AS's transmitter have send. When the message is decodified, we can separate PlantaPiloto to check if the message is correct. If PlantaPiloto is OK, we can start to create the final message. 

We send to the server all measures thanks to MQTT. The topic to send this message is: "Fabiano/"device"/t/RecSi". Device part is separated when message is received from transmitters. Then, "t" is the variable which contains all data. Finally, RecSi allows to save them in the server. 


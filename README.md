# pixki

[Pixki = Guardian] Bluetooth gatekeeper ???

Para un esquema de monitoreo, imaginemos que tenemos un sensor (S) y un observador (O):

     El S únicamente necesitará reportar al O, así que funcionará como Peripheral (Servidor)
     El O deberá obserbar diferentes S, así que funcionará como Central (Cliente)

Para una conexión de control remoto, imaginemos que tenemos un control remoto (RC) y un dispositivo controlado (CD):

     El CD únicamente debe ser controlado por un RC así que funcionará como un Perhipheral (Servidor)
         Esto además permitirá que el consumo de batería del CD sea menor
         
     El RC potencialmente podría controlar varios CD así que funcionará como un Central (Cliente)

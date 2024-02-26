# Async server example

How to write an async "server".
The server is offering two methods on the object "example".

- echo { "message":"str" } -> answer { "message": "str" }.
 if message is null hello world is given back.

- longecho { "message":"str" } -> answer { "message": "str" }.
 The server will wait async 5 seconds until answering.


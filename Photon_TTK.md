Things to Know (Particle Photon):

When using SparkJSON, read the Memory Model page. https://bblanchon.github.io/ArduinoJson/doc/memory/
Define JSON buffers inside methods.

Also, defining a json buffer that is too large may knock out the Photon. Try with one that's like 6Kb.

Things also get weird if you have a method with a reference to a JsonObject as its return type. Passing that object around
led to hard faults.

It's also seemingly helpful not to share http request and response objects globally for reuse.
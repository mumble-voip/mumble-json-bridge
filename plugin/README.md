# JSON-Bridge Mumble plugin

This project contains the Mumble plugin that will enable the JSON API functionality. In order to do so,
the plugin has to be installed into Mumble. After that, make sure that the plugin is actually loaded
from Mumble's settings.

If the plugin fails to initialize on a Unix-system, it could be that the last named pipe that has been created
did not get deleted (due to an improper termination of Mumble). In that case you have to delete the pipe
manually before starting the plugin. In order to do so, execute

```bash
rm /tmp/.mumble-json-bridge
```


# CLI

The CLI is the part that allows external applications to easily talk to Mumble's JSON bridge without having to
worry about the communication details (i.e. the process does not have to worry about named pipes and how
and which to use).

All requests are made using JSON syntax. The basic message format is
```
{
    "message_type": "<type>",
    "message": {
        <message body>
    }
}
```

Available choices for `<type>` are:

| **Type** | **Description** |
| -------- | --------------- |
| `api_call` | A direct call to one of Mumble's API functions (plugin API) |
| `operation` | A more high-level operation |

## api_call

In order to directly make an API call, the `<message body>` has to follow the form
```
"function": "<API function name>`,
"parameter": {
    <parameter>
}
```

`<parameter>` is a list of key-value-pairs providing the parameters for the function that is
going to be called. The necessary parameters (names and type) as well as the function name itself
can be obtained from the official
[C++ plugin API wrapper](https://github.com/mumble-voip/mumble-plugin-cpp/blob/7e1256a958d8e452ddb5273a20c30b0a26d6c4dc/include/mumble/plugin/MumbleAPI.h).
Note however that the parameter names have to be converted to snake_case first (e.g. `useID` transforms
to `user_id`).

If a function does not take any parameter, the entire `parameter` entry **must not be present** in
the sent message.

### Examples

```
{
    "message_type": "api_call",
    "message": {
        "function": "getActiveServerConnection"
    }
}
```

```
{
    "message_type": "api_call",
    "message": {
        "function": "getChannelName",
        "parameter": {
            "connection": 0,
            "channel_id": 5
        }
    }
}
```


## operation

Usually one needs to perform multiple successive API calls in order to get what one wants. This
is exactly where operations come into play. An operation is a specific API call combination that
is automatically processed by the CLI for you (including error handling). If for instance you want
to obtain the local user's name, this involves 3 API calls aready:
1. Obtain the ID of the current connection
2. Obtain local user's ID
3. Obtain the name associated with that ID

Thankfully there is an operation for this:
```
{
    "message_type": "operation",
    "message": {
        "operation": "get_local_user_name"
    }
}
```

Operations can also take parameter, which are handled in the same way as parameter for API calls:
```
{
    "message_type": "operation",
    "message": {
        "operation": "move",
        "parameter": {
            "user": "SomeUser",
            "channel": "MyChannel"
        }
    }
}
```

All operations are defined in the [operations](operations/) directory in form of a YAML file. It
contains the definition of the operation and also what parameter it takes.


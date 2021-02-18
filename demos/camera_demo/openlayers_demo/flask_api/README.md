# Camera Demo Flask API

## Installation
Requirements:
* Python 3
* Flask
* Flask CORS

`pip3 install -r requirements.txt`

## Run server
* Default host:port (localhost:5000)
  * `flask run`
* Custom host:port
  * `flask run -h <host> -p <port>`
  * If using a custom host/port, you must update the `locationUrl` in `openlayers_demo/src/app/location.service.ts` 
  
## API

### GET /
  * Returns 'Hello, World!'

### POST /location
  * Expects a JSON body conatining keys 'latitude' and 'longitude'
  * Stores the latitude, longitude
  * Stores the timestamp of when the request was processed
  * Returns 'ok'

### GET /location
  * If less than one second has passed since the most recent POST /location, returns {}
  * Otherwise returns the most recent latitude, longitude as JSON  
    ```
    {
      "latitude": <latitude>, 
      "longitude": <longitude>
    }
    ```

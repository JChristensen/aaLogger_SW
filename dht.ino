//dht.ino - Read a DHT22 temperature/humidify sensor.
//This sensor requires a two-second sample time. Call readDHT() after
//calling readDS18B20(). The latter sleeps for one second while the DS18B20
//does the conversion, readDHT() sleeps an additional second.

DHT dht(DHT_PIN, DHT_TYPE, 3);              //count=3 for 8MHz MCU

void readDHT(int *temp, int *rh)
{
    dht.begin();
    //sleep for another second so that the DHT has a total of two seconds
    wdtEnable();
    gotoSleep(true);
    wdtDisable();

    float t = dht.readTemperature(true);    //true --> fahrenheit
    float h = dht.readHumidity();

    if ( isnan(t) || isnan(h) ) {           //NaN indicates DHT read error
        *temp = *rh = -9999;
    }
    else {
        *temp = t * 10. + 0.5;
        *rh = h * 10. + 0.5;
    }       
}


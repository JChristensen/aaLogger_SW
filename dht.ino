//dht.ino - Read a DHT22 temperature/humidify sensor.
//This sensor requires a two-second sample time. Call readDHT() after
//calling readDS18B20(). The latter sleeps for one second while the DS18B20
//does the conversion, readDHT() sleeps an additional second.

DHT dht(DHT22_PIN, DHT_TYPE);

void readDHT(int *temp, int *rh)
{
    //sleep for another second so that the DHT has a total of two seconds
    wdtEnable();
    gotoSleep(true);
    wdtDisable();
    
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    // check if returns are valid, if they are NaN (not a number) then something went wrong!
    if ( isnan(t) || isnan(h) )
        *temp = *rh = -9999;
    else {
        *temp = 10. * (t + 0.05);
        *rh = 10. * (h + 0.05);
    }       
}


class Card
{
    constructor (url, text_element, sufix){
        this.url = url;
        this.sufix = sufix;
        this.text_element = text_element;
    }

    set_text (text){
        this.text_element.text(text);
    }
}

const cards = [new Card('api/measurements/latest?type=\'AIR_HUM\'', $('#air_hum_panel_text'), '%'),
new Card('api/measurements/latest?type=\'AIR_TEMP\'', $('#air_temp_panel_text'), '°C'),
new Card('api/measurements/latest?type=\'GND_HUM\'', $('#gnd_hum_panel_text'), '%'),
new Card('api/measurements/latest?type=\'LGT_INT\'', $('#lgt_int_panel_text'), 'lx')];

cards.forEach(card => 
{
    cards.push(fetchData(card.url, (r) => {card.set_text( Math.round(r[0].value).toString() + card.sufix);}));
});


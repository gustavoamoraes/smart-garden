CanvasRenderingContext2D.prototype.roundRect = function (x, y, w, h, r) 
{
    this.beginPath();
    this.moveTo(x - w/2 + r, y - h/2);
    this.arcTo(x + w/2, y - h/2, x + w/2, y + h/2, r);
    this.arcTo(x + w/2, y + h/2, x - w/2, y + h/2, r);
    this.arcTo(x - w/2, y + h/2, x - w/2, y - h/2, r);
    this.arcTo(x - w/2, y - h/2, x + w/2, y - h/2, r);
    return this;

}

ImageData.prototype.filterColor = function (color) 
{
    const rgb_color = hexToRgb('#9F87BF');
    const data = this.data;
    const is_in_margin = (n,v,m) => { return n >= v-m & n <= v+m;};

    for (var i = 0; i < data.length - 4; i += 4) 
    {
        if(!is_in_margin(data[i], rgb_color.r, 5) & !is_in_margin(data[i+1], rgb_color.g, 5) & !is_in_margin(data[i+2], rgb_color.b, 5))
        {
            data[i+3] = 0;
        }
    }

    return this;
}

function hexToRgb(hex) 
{
    var result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
    return result ? 
    {
      r: parseInt(result[1], 16),
      g: parseInt(result[2], 16),
      b: parseInt(result[3], 16)
    } : null;
}
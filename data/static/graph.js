var canvas = document.getElementById("graph_canvas");
var ctx = canvas.getContext("2d");

const initial_screen_res = [900,500];
var current_screen_res = [];
setCanvasRes(initial_screen_res);

var grid_rect = null;
var grid_size = null;

const get_grid_rect = () => {return [[left_margin, top_margin], [current_screen_res[0] - right_margin, current_screen_res[1] - bottom_margin]]};
const get_grid_size = () => {
    let grid_rect = get_grid_rect();

    return [grid_rect[1][0] - grid_rect[0][0], grid_rect[1][1] - grid_rect[0][1]]
};

//Graph settings
const graph_line_width = 1;
const grid_line_width = 1;
const horizontal_divisions = 10;
const minute_intervals = {30: 1440*3, 7: 1440, 1 : 120};
const date_formats = {30: { day: 'numeric' , month: 'short'}, 7: { day: 'numeric' , month: 'long'}, 1: { hour: 'numeric' , minute: 'numeric'}};

//Graph parameters
var days_interval = 1;
var current_time_lenght = days_interval*24*60;
const max_time_lenght = 30*24*60;

//Dates
var current_date = new Date();
var start_date = new Date(current_date.getTime() - (days_interval*24*60*60*1000));
var month_ago = new Date(current_date.getTime() - (30*24*60*60*1000));

//Margins
const bottom_margin = 50;
const top_margin = 15;
const default_text_margin = 70;
const timeline_margin = 25;
var right_margin = 0;
var left_margin = 0;

//
var canvas_snapshot = null;

//Graph fuctions
const yPos = (scale_range, value) => 
{
    return (current_screen_res[1] - (current_screen_res[1] - top_margin - bottom_margin) / (scale_range[1] - scale_range[0])  * (value - scale_range[0])) - bottom_margin;
};

const xPos = (minutes) => 
{
    return left_margin + (current_screen_res[0] - right_margin - left_margin) / current_time_lenght * minutes
};

//Elements
const interval_select = document.getElementById('interval_options');
const check_boxes = [$("#air_temp_check"), $("#air_h_check"), $("#earth_h_check"), $("#light_check")];

//Events
interval_select.onchange = () => {onChangedInterval();};
check_boxes.map(e => e.click(() => {drawGraph();}));

//Data set types
class MeasurementSet
{
    constructor(scale_range, scale_symbol, isActive, measurement_type, name, graph_color) 
    {
        this.dataset = [];
        this.labeledDataset = {};
        this.name = name;
        this.scale_range = scale_range;
        this.scale_symbol = scale_symbol;
        this.graph_color = graph_color;
        this.label_color = graph_color;
        this.isActive = isActive;
        this.measurement_type = measurement_type;
    }
    
    setDataset(dataset)
    {
        this.dataset = dataset;
    }

    findNextObjTo(x)
    {
        for (let i = x; i <= max_time_lenght; i++)
        {
            const measurement = this.labeledDataset[i];
            if(typeof measurement == "undefined") continue;
            return measurement;
        }

        return null;
    }

    labelDataset(key)
    {
        for (let i = 0; i < this.dataset.length; i++)
        {
            this.labeledDataset[key(this.dataset[i])] = this.dataset[i];
        }
    }
}

const measurement_types = [new MeasurementSet([0,30],'°C', () => { return check_boxes[0].is(":checked")}, 'AIR_TEMP', "Temperatura do ar", '#9F87BF'),
                        new MeasurementSet([10,100],'%', () => { return check_boxes[1].is(":checked")}, 'AIR_HUM', "Umidade do ar", '#53BDB7'),
                        new MeasurementSet([0,100],'%', () => { return check_boxes[2].is(":checked")}, 'GND_HUM', "Umidade da terra", '#EA9363'),
                        new MeasurementSet([0,3000],'lx', () => { return check_boxes[3].is(":checked")}, 'LGT_INT', "Intensidade da luz", '#F0CE69')];

//Promices for each set
let data_promices = [];

//Make api request and download all the data needed
measurement_types.forEach(type => 
{
    data_promices.push(fetchData('api/measurements?type=\''+type.measurement_type+'\'&date__gt=\''+month_ago.toISOString() + '\'', (r) => {type.setDataset(r)}));
});

//Draw graph when all data is downloaded
Promise.all(data_promices).then(() => 
{
    onFinishDownload();
});

var active_sets = [];

function update_active_sets()
{ 
    measurement_types.forEach(type =>
    {   
        let includes = active_sets.includes(type);

        if(type.isActive())
        {
            if(!includes) active_sets.push(type);
        }
        else if (includes)
        {
            active_sets.splice(active_sets.indexOf(type), 1);
        }
    });
}

function onFinishDownload() 
{
    measurement_types.forEach(type => 
    {
        type.labelDataset((e) => {return parseInt((new Date(e.date) - month_ago)/60000, 10);});
    });

    console.log(measurement_types);

    drawGraph();   
}

function onChangedInterval()
{
    days_interval = parseInt(interval_select.options[interval_select.selectedIndex].value);
    current_time_lenght =  days_interval * 1440;
    start_date = new Date(current_date.getTime() - (days_interval*24*60*60*1000));

    drawGraph();
}

function setCanvasRes(res) 
{
    canvas.width = res[0]
    canvas.height = res[1]
    return res;
}

function drawGraph()
{
    //Clear screen
    ctx.clearRect(0, 0, canvas.width, canvas.height);

    //Get types that are checked
    update_active_sets();

    //Reset margins
    left_margin = active_sets.length >= 1? 0 : timeline_margin;
    right_margin = active_sets.length >= 2? 0 : timeline_margin;
    
    //New resolution to make grid size constant
    let new_resolution = [initial_screen_res[0] + (active_sets.length*default_text_margin), initial_screen_res[1]];
    current_screen_res = setCanvasRes(new_resolution);

    //Margins
    let scale_margins = [0,0]; //left and right
    let scales_x = Array(active_sets.length);
    
    for (let i = 0, j = 0; i < active_sets.length; i++, j = i%2) 
    {
        scale_margins[j] += default_text_margin;

        if(j == 0)
        {
            scales_x[i] = scale_margins[j] - default_text_margin/2;
            left_margin += default_text_margin;
        }
        else
        {
            scales_x[i] = new_resolution[0] - scale_margins[j] + default_text_margin/2;
            right_margin += default_text_margin;
        }
    }

    //Update grid rect
    grid_rect = get_grid_rect();
    grid_size = get_grid_size();

    //Draw everything
    drawGridAndLabels();
    
    for (let i = 0; i < active_sets.length; i++) 
    {
        //Draw active scales labels
        drawScaleLabels(active_sets[i], scales_x[i]);

        //plot data on graph
        plotValues(active_sets[i]);
    }

    canvas_snapshot = ctx.getImageData(0,0, current_screen_res[0], current_screen_res[1]);
}

function drawGridAndLabels() 
{
    ctx.beginPath();

    for(var i = 0; i <= horizontal_divisions;i++)
    {   
        let y_value =  1 / horizontal_divisions * i;
        let y_pos = Math.floor(yPos([0,1], y_value)) + .5;

        ctx.moveTo(grid_rect[0][0], y_pos);
        ctx.lineTo(grid_rect[1][0], y_pos);
    }

    const interval = minute_intervals[days_interval];
    const minutes_since_midnight = (current_date.getHours()*60) + current_date.getMinutes();
    const interval_accumulator = (Math.ceil(minutes_since_midnight/interval) * interval) - minutes_since_midnight;

    for(var i = interval_accumulator; i <= current_time_lenght; i+=interval)
    {   
        let x_pos =  Math.floor(xPos(i)) + .5;

        ctx.moveTo(x_pos, grid_rect[0][1]);
        ctx.lineTo(x_pos, grid_rect[1][1]);

        //Draw label for this minute
        drawTimeLabel(x_pos, i);
    }

    ctx.strokeStyle = 'rgb(200,200,200)';
    ctx.lineWidth = grid_line_width;
    ctx.stroke();
}

function plotValues(data_type)
{
    let data_set_lenght = Object.keys(data_type.dataset).length;

    if(data_set_lenght < 2)
    {
        return;
    }

    const initial_point = (max_time_lenght - current_time_lenght);

    ctx.beginPath();

    for (let i = 0; i < current_time_lenght; i++)
    {
        const measurement = data_type.labeledDataset[initial_point + i];
        if(typeof measurement == "undefined") continue;
        const measurement_value = measurement['value'];
        const graph_pos = [xPos(i), yPos(data_type.scale_range, measurement_value)];
        ctx.lineTo(graph_pos[0], graph_pos[1]);
    }

    ctx.strokeStyle = data_type.graph_color;
    ctx.lineWidth = graph_line_width;
    ctx.stroke()
}

function drawTimeLabel(x, minute)
{
    const date = new Date(start_date.getTime() + (minute*60*1000)); //Sum milliseconds to the start date
    const date_formated = date.toLocaleString('pt-br', date_formats[days_interval]);

    ctx.font = "15px Segoe UI"
    ctx.textAlign = "center";
    ctx.fillStyle = '#000000';
    ctx.textBaseline = 'middle';
    ctx.fillText(date_formated, x, current_screen_res[1] - bottom_margin / 2);
}
    
function drawScaleLabels(set, x_pos)
{
    for(var i = 0; i <= horizontal_divisions;i++)
    {   
        let y_value = set.scale_range[0] + (set.scale_range[1] - set.scale_range[0])/horizontal_divisions * i; 
        let y_pos = yPos(set.scale_range, y_value);

        ctx.font = "15px Segoe UI"
        ctx.textAlign = "center";
        ctx.fillStyle = set.label_color;
        ctx.textBaseline = 'middle';
        ctx.fillText(String(Math.round(y_value)) + set.scale_symbol, x_pos, y_pos);
    }

    //Side line
    var middle_dir = Math.sign(current_screen_res[0]/2 - x_pos);
    let line_x_pos = Math.floor(x_pos + default_text_margin/2 * middle_dir) +.5;

    ctx.beginPath();

    ctx.moveTo(line_x_pos, yPos([0,1],0));
    ctx.lineTo(line_x_pos, yPos([0,1],1));

    ctx.lineWidth = grid_line_width;
    ctx.strokeStyle = set.label_color;
    ctx.stroke();
}

class Element 
{
    constructor ()
    {
        this.bottom_margin = 0;
        this.top_margin = 0;
        this.left_margin = 0;
        this.right_margin = 0;

        this.canvas_position = [0, 0];
    }

    setCanvasPosition (pos) {this.canvas_position = pos;}

    get height () {}
    get width () {}
}

class TextElement extends Element 
{
    constructor (left_margin, right_margin, top_margin, bottom_margin, text, font, font_size)
    {   
        super();

        this.left_margin = left_margin;
        this.right_margin = right_margin;
        this.top_margin = top_margin;
        this.bottom_margin = bottom_margin;

        this.text = text;
        this.font = font;
        this.font_size = font_size;
    }

    draw ()
    {
        ctx.save();
        ctx.font = this.font;
        ctx.fillStyle = 'black';
        ctx.fillText(this.text, this.canvas_position[0], this.canvas_position[1]);
        ctx.restore();
    }

    get height () { return this.font_size}

    get width () 
    {
        ctx.save();
        ctx.font = this.font;
        let width = ctx.measureText(this.text).width; 
        ctx.restore();
        return width;
    }
}

class ColorBoxElement extends Element 
{
    constructor (color, size)
    {   
        super();

        this.color = color;
        this.size = size
    }

    draw ()
    {
        ctx.save();
        ctx.roundRect(this.canvas_position[0] , this.canvas_position[1], this.size, this.size, 5);
        ctx.fillStyle = this.color;
        ctx.fill();
        ctx.restore();
    }

    get height () {return this.size;}
    get width () { return this.size; }
}

class LabelElement extends Element
{
    constructor (left_margin, right_margin, top_margin, bottom_margin, text, label_color)
    {   
        super();
        
        this.left_margin = left_margin;
        this.right_margin = right_margin;
        this.top_margin = top_margin;
        this.bottom_margin = bottom_margin;

        this.color_box_element = new ColorBoxElement(label_color, 20);
        this.text_element = new TextElement(5, 0, 0, 0, text, "15px Segoe UI", 15);
    }

    get height () 
    { 
        return Math.max(this.color_box_element.height, this.text_element.height);
    }

    get width () 
    { 
        return this.color_box_element.width + this.text_element.width +
        this.color_box_element.left_margin + this.text_element.right_margin +
        this.text_element.left_margin + this.text_element.right_margin;
    }

    draw () 
    {
        const color_box_pos = [this.canvas_position[0] - (this.width/2) + this.color_box_element.width/2, this.canvas_position[1]]
        const text_element_pos = [this.text_element.left_margin + color_box_pos[0] + (this.color_box_element.width/2) + this.text_element.width/2, this.canvas_position[1]]

        this.color_box_element.setCanvasPosition(color_box_pos)
        this.text_element.setCanvasPosition(text_element_pos);
    
        this.color_box_element.draw();
        this.text_element.draw();
    }
}

class ArrowBoxElement extends Element 
{
    constructor (arrow_height, arrow_width, direction)
    {   
        super();

        this.arrow_height = arrow_height;
        this.arrow_width = arrow_width;
        this.direction = direction;

        this.child_elements = [];
    }

    addElementAndRecalculatePositions(element)
    {
        this.child_elements.push(element);

        //Element pos 
        let current_pos = [this.box_center[0] - (this.width/2), this.box_center[1] - this.height/2];

        this.child_elements.forEach(element => 
        {
            current_pos[1] +=  element.top_margin + element.height + element.bottom_margin;

            element.setCanvasPosition([element.left_margin + current_pos[0] + (element.width/2), 
            current_pos[1] - element.bottom_margin - (element.height/2)]);
        });

    }

    get box_center () 
    { 
        return [this.canvas_position[0] - (this.arrow_height + this.width/2) * this.direction[0], 
        this.canvas_position[1] - (this.arrow_height + this.height/2) * this.direction[1]];
    }

    get height () 
    {
        return this.child_elements.reduce((sum, e) => sum + (e.height + e.top_margin + e.bottom_margin), 0);
    }

    get width () 
    {
        let max_width = 0;
        
        this.child_elements.forEach(element => 
        {
            let element_width = element.left_margin + element.right_margin + element.width;
            if(element_width  > max_width) max_width = element_width;
        });

        return max_width;
    }

    draw ()
    {
        //Round box
        ctx.roundRect(this.box_center[0], this.box_center[1], this.width, this.height, 5);

        //Spike
        ctx.moveTo(this.canvas_position[0] - (this.arrow_height * this.direction[0]) - (this.arrow_width/2 * this.direction[1]), 
        this.canvas_position[1] - (this.arrow_height * this.direction[1]) - (this.arrow_width/2 * this.direction[0]));

        ctx.lineTo(this.canvas_position[0], this.canvas_position[1]);

        ctx.lineTo(this.canvas_position[0] - (this.arrow_height * this.direction[0]) + (this.arrow_width/2 * this.direction[1]), 
        this.canvas_position[1] - (this.arrow_height * this.direction[1]) + (this.arrow_width/2 * this.direction[0]));
        //

        ctx.save();

        ctx.strokeStyle = 'rgb(100,100,100)';
        ctx.fillStyle = 'rgb(255,255,255)';
        ctx.lineWidth = 1;
        
        ctx.shadowColor = "rgb(200,200,200)";
        ctx.shadowBlur = 3;
        ctx.shadowOffsetX = 1;
        ctx.shadowOffsetY = 1;

        ctx.stroke();
        ctx.fill();

        ctx.restore();

        this.child_elements.forEach(element => 
        {
            element.draw();
        });
    }
}

function isInsideGrid(posX, posY) 
{
    if(grid_rect == null) return false;
    return posY > grid_rect[0][1] & posY < grid_rect[1][1] & posX > grid_rect[0][0] & posX < grid_rect[1][0];
}

function drawDashedLines (mouse_pos)
{
    ctx.save();

    // Dashed line
    ctx.beginPath();
    ctx.setLineDash([2, 2]);
    ctx.moveTo(grid_rect[0][0], mouse_pos[1] + .5);
    ctx.lineTo(grid_rect[1][0], mouse_pos[1] + .5);
    ctx.moveTo(mouse_pos[0] , grid_rect[0][1] + .5);
    ctx.lineTo(mouse_pos[0] , grid_rect[1][1] + .5);

    ctx.lineWidth = 1;
    ctx.strokeStyle = 'rgb(150,150,150)';
    ctx.stroke();

    ctx.restore();
}

(function() 
{
    document.onmousemove = handleMouseMove;

    function handleMouseMove(event) 
    {   
        if(canvas_snapshot != null) ctx.putImageData(canvas_snapshot,0,0);

        const c_rect = canvas.getBoundingClientRect();
        const canvas_relative_pos = [event.screenX - c_rect.x, event.screenY - c_rect.y - 100];
        const grid_relative_pos = [event.screenX - c_rect.x - left_margin, event.screenY - c_rect.y - 100];

        if(canvas_relative_pos != null && isInsideGrid(canvas_relative_pos[0], canvas_relative_pos[1]))
        {
            //Dashed lines
            drawDashedLines(canvas_relative_pos)

            const minute = (max_time_lenght - current_time_lenght) + Math.floor(grid_relative_pos[0]/grid_size[0] * current_time_lenght);

            let box_groups = [];

            active_sets.forEach(dataset => 
            {
                const clossest_obj = dataset.findNextObjTo(minute);

                if (clossest_obj == null) return;

                const value = Math.floor(clossest_obj.value);
                const point = [canvas_relative_pos[0], yPos(dataset.scale_range, clossest_obj.value)];
                const dir = [point[0] > current_screen_res[0]/2 ? 1 : -1, 0];

                const new_element = new LabelElement(5,5,5,5, dataset.name + ": " + value.toString() + dataset.scale_symbol, dataset.graph_color);
                const new_arrow_box = new ArrowBoxElement(10, 10, dir);
                new_arrow_box.setCanvasPosition(point);
                new_arrow_box.addElementAndRecalculatePositions(new_element);

                for (let i = 0; i < box_groups.length; i++) 
                {
                    const arrow_box = box_groups[i].arrow_box;
                    const box_group = box_groups[i];

                    if(Math.abs(point[1] - arrow_box.box_center[1]) < arrow_box.height + new_arrow_box.height)
                    {
                        box_group.data_points.push(point);
                        let avg_y = box_group.data_points.reduce((sum, point) => sum + point[1], 0)/box_group.data_points.length;
                        arrow_box.setCanvasPosition([point[0], avg_y]);
                        arrow_box.addElementAndRecalculatePositions(new_element);
                        return;
                    }
                }

                box_groups.push(
                {
                    arrow_box: new_arrow_box,
                    data_points: [point],
                });
            });

            box_groups.forEach(box_group => 
            {
                box_group.arrow_box.draw();
            });

            //Time box
            let time_box = new ArrowBoxElement(10, 10, [0,-1]);
            const mouse_date = new Date(month_ago.getTime() + (minute*60*1000));
            const date = mouse_date.toLocaleString('pt-br', date_formats[days_interval]);
            const text_element = new TextElement(10, 10, 10, 10, date, "15px Segoe UI", 15);
            time_box.setCanvasPosition([canvas_relative_pos[0], grid_rect[1][1]]);
            time_box.addElementAndRecalculatePositions(text_element);
            time_box.draw();
        }
    }
})();
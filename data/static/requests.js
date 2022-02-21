const base_url = '/';

async function fetchData (url, callback)
{
    const response = await fetch(base_url + url)
    .catch(error =>
    {
        console.log('Fetch Error : ', error);
    });
   
    if (response.status !== 200) 
    {
        console.log('Looks like there was a problem. Status Code: ' + response.status);
        return;
    }

    const data = await response.json()

    callback(data);
}
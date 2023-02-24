kwsContext = {
    data: {
        method: "search",
        params: {
            type: 0,
            search: "",
            page: 1,
            page_size: 10
        }
    },
    model: [],
    fetch: (callback) => {
        let r = new XMLHttpRequest();

        r.addEventListener("load", () => {
            kwsContext.r = r.responseText;

            callback();
        });

        r.open("POST", "/search.cgi", true);

        r.send(JSON.stringify(kwsContext.data));
    },
    update: () => {
        let c = document.getElementById('results');

        c.innerHTML = kwsContext.r;
    }
};

function addExecuteAfterInputDelayHandler(element, f)
{
    element.addEventListener("input", () => {
        clearTimeout(element.timerId);

        element.timerId = setTimeout(f, 300);
    });
}

function addAppEventHandlers()
{
    let kwsInput = document.getElementById('kws');

    addExecuteAfterInputDelayHandler(kwsInput, () => {
        kwsContext.data.params.search = kwsInput.value;

        kwsContext.fetch(() => {
            kwsContext.update();
        });
    });

}

addEventListener("load", () => {  });

addEventListener("DOMContentLoaded", addAppEventHandlers);
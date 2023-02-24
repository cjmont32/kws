kwsContext = {
    data: {
        method: "search",
        params: {
            type: 0,
            search: "",
            page: 1,
            page_size: 1000
        }
    },
    response: "",
    responseObj: {},
    model: [],
    fetch: (callback) => {
        let r = new XMLHttpRequest();

        r.addEventListener("load", () => {
            if (!r.responseText)
                return;

            kwsContext.response = r.responseText;

            kwsContext.responseObj = JSON.parse(kwsContext.response);

            if (kwsContext.responseObj.error) {
                return;
            }

            if (kwsContext.responseObj.result) {
                kwsContext.model = kwsContext.responseObj.result;
            }

            callback();
        });

        r.open("POST", "/search.cgi", true);

        r.send(JSON.stringify(kwsContext.data));
    },
    update: () => {
        let r = document.getElementById('results');

        while (r.hasChildNodes()) {
            r.firstChild.remove();
        }

        addQuestions(r);
    }
};

function addQuestions(node)
{
    for (let i = 0; i < kwsContext.model.length; i++) {
        node.appendChild(createQuestionDOMObj(kwsContext.model[i]));
    }
}

function createQuestionDOMObj(item)
{
    let qNode, alNode, aNode;

    qNode = document.createElement("div");

    qNode.setAttribute("class", "question");

    qNode.appendChild(document.createTextNode(item.question));

    alNode = document.createElement("div");

    alNode.setAttribute("class", "answer_list");

    for (let i = 0; i < item.answers.length; i++) {
        aNode = document.createElement("div");

        aNode.setAttribute("class", "answer");

        aNode.appendChild(document.createTextNode(item.answers[i]));

        alNode.appendChild(aNode);
    }

    qNode.appendChild(alNode);

    return qNode;
}

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

addEventListener("DOMContentLoaded", addAppEventHandlers);
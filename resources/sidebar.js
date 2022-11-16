(function() {
    const sidebar = document.querySelector("#sidebar");
    const html = document.querySelector("html");
    const main = document.querySelector("main");
    const collapse = document.createElement("button");
    const lastItem = document.querySelector("ul");
    const darkToggle = document.createElement("button");
    const body = document.querySelector("body");
    darkToggle.classList.add("dark-toggle");
    darkToggle.textContent = "Light Mode"
    lastItem.after(darkToggle);
    collapse.classList.add("collapse");
    const initialText = "<<";
    collapse.textContent = initialText;
    main.parentNode.insertBefore(collapse, main);
    const root = document.querySelector(':root');
    collapse.addEventListener("click", () => {
        if (localStorage.getItem("darkMode") == "Dark") {
            body.classList.toggle("hide-color");
        }
        if (localStorage.getItem("collapse") == "Collapsed") {
            collapse.textContent = ">>";
            collapse.classList.remove("left")
            sidebar.classList.remove("hide");
            localStorage.setItem("collapse", "Expanded");
        } else {
            localStorage.setItem("collapse", "Collapsed");
            sidebar.classList.add("hide");
            collapse.classList.add("left")
            collapse.textContent = initialText;
        }
    });

    if (localStorage.getItem("darkMode") == "Dark") {
        setLight();
    } else {
        setDark();
    }

    if (localStorage.getItem("collapse") == "Expanded") {
        sidebar.classList.remove("hide");
        collapse.textContent = ">>";
        collapse.classList.remove("left")
    } else {
        sidebar.classList.add("hide");
        if (localStorage.getItem("darkMode") == "Dark") {
            body.classList.add("hide-color");
        }
        collapse.classList.add("left")
        collapse.textContent = initialText;
    }

    function setStyles() {
        if (localStorage.getItem("darkMode") == "Light") {
            localStorage.setItem("darkMode", "Dark")
            setLight()
        } else {
            localStorage.setItem("darkMode", "Light");
            setDark();
        }
    }

    function setLight() {
        darkToggle.textContent = "Dark Mode";
        root.style.setProperty('--mainColor', '#FFF');
        root.style.setProperty('--secondMain', '#fff');
        root.style.setProperty('--textColor', '#000');
        root.style.setProperty('--sidebar-back', '#EEE');
        root.style.setProperty('--sidebar-foreground', '#fff');
        root.style.setProperty('--scrollbar', '#EEE');
        root.style.setProperty('--scrollbarThumb', '#C1C1C1');
        root.style.setProperty('--linkColor', '#03E');
        root.style.setProperty('--string', '#292');
        root.style.setProperty('--linkVisited', '#938');
        root.style.setProperty('--aliasName', 'darkgoldenrod');
    }

    function setDark() {
        darkToggle.textContent = "Light Mode";
        root.style.setProperty('--mainColor', '#3D3D3D');
        root.style.setProperty('--secondMain', '#333333');
        root.style.setProperty('--textColor', 'white');
        root.style.setProperty('--sidebar-back', '#333333');
        root.style.setProperty('--sidebar-foreground', '#3D3D3D');
        root.style.setProperty('--scrollbar', '#333333');
        root.style.setProperty('--scrollbarThumb', '#3D3D3D');
        root.style.setProperty('--linkColor', '#00C2EE');
        root.style.setProperty('--string', '#39B339');
        root.style.setProperty('--linkVisited', '#E77BD5');
        root.style.setProperty('--aliasName', 'gold');
    }
    darkToggle.addEventListener("click", () => {
        setStyles()
    });
    let node = document.querySelector("#sidebar>ul");
    let items = node.querySelectorAll("li");
    let found = null;
    for (let item of items) {
        if (!found) {
            //
            // If a sidebar item links to the page we're already on, identify 
            // it with a CSS class so we can style it differently.
            //
            let link = item.firstElementChild;
            if (link.href == document.location.href) {
                link.classList.add("you-are-here");
                found = item;
            }
        }
        if (item.hasAttribute("data-disallow-collapse"))
            continue;
        //
        // Set up expand/collapse buttons:
        //
        let sub = item.querySelector("ul");
        if (item.hasAttribute("data-delete-if-empty")) {
            if (!sub || !sub.children.length) {
                item.parentNode.removeChild(item);
                continue;
            }
        }
        if (!sub)
            continue;
        let toggle = document.createElement("a");
        toggle.classList.add("toggle");
        item.insertBefore(toggle, item.children[0]);
    }
    node.addEventListener("click", function(e) {
        //
        // Click handler for expand/collapse buttons.
        //
        let toggle = e.target.closest(".toggle");
        if (!toggle)
            return true;
        e.preventDefault();
        let item = toggle.parentNode;
        item.classList.toggle("collapsed");
    });
    //
    if (window.page_load_start_time && found) {
        if (found.parentNode.closest("li") || found.querySelector("ul")) {
            //
            // Auto-collapse sidebar navigation elements that we're not "in," 
            // but only if the following conditions are met:
            //
            //  - The page loaded quickly.
            //  - We know where in the nav we are.
            //  - The place we're at is a child, OR it has children.
            //
            let done = Date.now();
            if (done - window.page_load_start_time < 1000) {
                for (let item of items) {
                    if (item == found)
                        continue;
                    if (item.hasAttribute("data-dont-default-collapse"))
                        continue;
                    if (item.hasAttribute("data-disallow-collapse"))
                        continue;
                    let sub = item.querySelector("ul");
                    if (!sub)
                        continue;
                    if (item.contains(found) || found.contains(item))
                        continue;
                    item.classList.add("collapsed");
                }
            }
        }
    }



     const getSiblings = function (elem) {

        // Setup siblings array and get the first sibling
        var siblings = [];
        var sibling = elem.firstChild;
    
        // Loop through each sibling and push to the array
        while (sibling) {
            if (sibling.nodeType === 1 && sibling !== elem) {
                siblings.push(sibling);
            }
            sibling = sibling.nextSibling
        }
    
        return siblings;
    
    };


    function setVariableColor() {
        const keyword = document.querySelectorAll(".keyword");
        keyword.forEach(word => {
          if (word.innerHTML == "alias") {
            let aliasName = word.nextSibling.nodeValue
            //This is the actual name of the variable
            
            if (aliasName == null) {
                return;
            }
            aliasName = aliasName.split("=")[0]
            
            //This will set the node value after the word alias to the portion after the equals sign, not including the alias name
            //word.nextSibling.nodeValue = word.nextSibling.nodeValue.substring(word.nextSibling.nodeValue.indexOf("="));
            
            //append the alias name span before the value of the alias name
            //word.after(newSpan)
            const pre = document.querySelectorAll("pre");
            pre.forEach(element => {
                let siblings = getSiblings(element);
                siblings.forEach(sibling => {
                    try {
                        let followingText = sibling.nextSibling.nodeValue;
                        let textNode = sibling.nextSibling;
                        let found = textNode.nodeValue.match(aliasName.trim());
                        if (found) {
                            console.log(aliasName.trim())
                            let new_str = followingText.split(aliasName.trim())[0] + aliasName;
                            new_str = new_str.split("=")[0]
                            let newSpan = document.createElement("span");
                            newSpan.classList.add("aliasName");
                            //set the html of the span to the actual name of the alias, not including its value with the equals sign
                            newSpan.innerHTML = aliasName;
                            console.log(newSpan)
                            //textNode.nodeValue = "=" + followingText.split("=")[1];
                            let splitText = followingText.split("=");
                            if (splitText[1].includes(aliasName.trim())) {
                                textNode.nodeValue = followingText.replace(aliasName.trim(), "");
                                textNode.after(newSpan)
                                //newSpan.parentNode.insertBefore(textNode, newSpan)
                                //  textNode.nodeValue = textNode.nodeValue.replace(/(\r\n|\n|\r)/, "");
                                //textNode.nodeValue = textNode.nodeValue.replace("alias", "\n alias");
                            } else {
                                spaces = splitText[0].search("\s+")
                                console.log(spaces)
                                textNode.nodeValue = followingText.replace(aliasName.trim(), "");
                                sibling.after(newSpan)
                            }
                            console.log(textNode.nodeValue)
                        }
                    } catch (ex) {console.log(ex)}                    
                })
            })
        }  
        })
        
    }

    window.onload = function() {
        sidebar.classList.add("afterload");
        html.classList.add("afterload");
        //setVariableColor()
    }
})();
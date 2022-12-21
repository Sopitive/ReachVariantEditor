(function() {
    const sidebar = document.querySelector("#sidebar");
    const html = document.querySelector("html");
    const main = document.querySelector("main");
    const collapse = document.createElement("button");
    const darkToggle = document.querySelector("#dark-mode");
    const preElements = document.querySelectorAll("pre");
    const modelCodeDisplay = document.createElement("pre");
    const dropdowns = document.querySelectorAll(".dropdown");
    const scrollbar = document.querySelector(".scrollbar");
    const mainHtml = document.querySelector("html");
    let scrollbarPos = ""
    let initialY = "";
    let currentY = "";
    let scrolling = false;
    let _docHeight = (document.height !== undefined) ? document.height : document.body.offsetHeight;
    let limit = Math.max( document.body.scrollHeight, document.body.offsetHeight, document.documentElement.clientHeight, document.documentElement.scrollHeight, document.documentElement.offsetHeight );
    let height = _docHeight/100;
    scrollbar.style = `height: ${72000/limit}%;`;
    
    modelCodeDisplay.classList.add("code-display");
    document.body.appendChild(modelCodeDisplay);
    const closeButton = document.createElement("button");
    closeButton.textContent = "X";
    closeButton.classList.add("close-button");
    closeButton.addEventListener('click', () => {
        modelCodeDisplay.style = "opacity: 0;";
        closeButton.style.display = "none";
    });


    window.onscroll = () => {
        let scrollPosition = mainHtml.scrollTop;
        scrollbarPos = ((scrollPosition/limit)*100)
        scrollbar.style = `top: ${scrollbarPos}%; height: ${74000/limit}%;`;
    }

    scrollbar.addEventListener('mousedown', (e) => {
        initialY = e.clientY;
        scrolling = true;
        e.preventDefault()
    })

    mainHtml.addEventListener('mouseup', () => {
        initialY = ""
        scrolling = false;
    })

    mainHtml.addEventListener('mousemove', (e) => {
        currentY = e.clientY;
        if (scrolling) {
            let adjustmentY = initialY - currentY
            window.scrollBy(0, adjustmentY * limit / -750)
            initialY = currentY
        }
    })

    modelCodeDisplay.addEventListener('click', (e) => {
        e.stopPropagation();
    });
    document.body.addEventListener('click', () => {
        modelCodeDisplay.style = "opacity: 0;";
        closeButton.style.display = "none";
    });
    document.body.appendChild(closeButton);
    //darkToggle.classList.add("dark-toggle");
    //darkToggle.textContent = "Light Mode";
    //body.appendChild(darkToggle);
    collapse.classList.add("collapse");
    const initialText = "Ξ";
    collapse.textContent = initialText;
    main.parentNode.insertBefore(collapse, main);
    const root = document.querySelector(':root');

    preElements.forEach(element => {
        element.addEventListener('click', (e) => {
            modelCodeDisplay.style = "opacity: 1; width: 90%; left: 50%; top: 50%";
            modelCodeDisplay.innerHTML = element.innerHTML;
            closeButton.style.display = "block";
            e.stopPropagation()
        });
        let copyButton = document.createElement("button");
        copyButton.classList.add("copy-button");
        copyButton.name = "copyButton";
        let icon = document.createElement("i");
        icon.classList.add("fa");
        icon.classList.add("fa-copy");
        copyButton.appendChild(icon);
        let text = element.textContent;
        const copyContent = async () => {
            try {
              await navigator.clipboard.writeText(text);
              icon.classList.remove("fa-copy");
              icon.classList.add("fa-check");
            } catch (err) {
              console.error('Failed to copy: ', err);
            }
          }
        copyButton.addEventListener('click', (e) => {
            e.stopPropagation();
            copyContent();
        });
        copyButton.addEventListener('focusout', () => {
            icon.classList.remove("fa-check");
            icon.classList.add("fa-copy");
        })
        element.appendChild(copyButton);
    });

    sidebar.addEventListener('mouseleave', () => {
        localStorage.setItem("collapse", "Expanded");
        sidebar.classList.add("hide");
        collapse.classList.add("left");
        collapse.textContent = initialText;
    })

    sidebar.addEventListener('mouseover', () => {
        localStorage.setItem("collapse", "Collapsed");
        sidebar.classList.remove("hide");
        collapse.classList.remove("left");
    })

    collapse.addEventListener("click", () => {
        if (localStorage.getItem("collapse") == "Expanded") {
            collapse.textContent = "Ξ";
            collapse.classList.remove("left")
            sidebar.classList.remove("hide");
            localStorage.setItem("collapse", "Collapsed");
        } else {
            localStorage.setItem("collapse", "Expanded");
            sidebar.classList.add("hide");
            collapse.classList.add("left")
            collapse.textContent = initialText;
        }
    });

    if (localStorage.getItem("darkMode") == "Light") {
        setLight();
    } else {
        setDark();
    }

    if (localStorage.getItem("collapse") == "Collapsed") {
        sidebar.classList.remove("hide");
        collapse.textContent = "Ξ";
        collapse.classList.remove("left")
    } else if (localStorage.getItem("collapse") == "Expanded") {
        sidebar.classList.add("hide");
        collapse.classList.add("left");
        collapse.textContent = initialText;
    } else {
        localStorage.setItem("collapse", "Expanded");
        sidebar.classList.add("hide");
        collapse.classList.add("left");
    }

    function setStyles() {
        if (localStorage.getItem("darkMode") == "Light") {
            setLight()
        } else {
            setDark();
        }
    }

    function setLight() {
        localStorage.setItem("darkMode", "Light");
        root.style.setProperty('--mainColor', '#FFF');
        root.style.setProperty('--secondMain', '#fff');
        root.style.setProperty('--textColor', '#000');
        root.style.setProperty('--sidebar-back', '#EEE');
        root.style.setProperty('--sidebar-foreground', '#fff');
        root.style.setProperty('--scrollbar', '#EEE');
        root.style.setProperty('--scrollbarThumb', '#C1C1C1');
        root.style.setProperty('--linkColor', '#03E');
        //root.style.setProperty('--string', '#292');
        root.style.setProperty('--linkVisited', '#938');
        document.querySelectorAll("pre").forEach(element => {
            element.classList.add("light");
        })
        //root.style.setProperty('--aliasName', 'darkgoldenrod');
    }

    function setDark() {
        darkToggle.checked = "true";
        localStorage.setItem("darkMode", "Dark");
        root.style.setProperty('--mainColor', '#3D3D3D');
        root.style.setProperty('--secondMain', '#333333');
        root.style.setProperty('--textColor', 'white');
        root.style.setProperty('--sidebar-back', '#2d2d2d');
        root.style.setProperty('--sidebar-foreground', '#3D3D3D');
        root.style.setProperty('--scrollbar', '#333333');
        root.style.setProperty('--scrollbarThumb', '#3D3D3D');
        root.style.setProperty('--linkColor', '#00C2EE');
        //root.style.setProperty('--string', '#26ff00');
        root.style.setProperty('--linkVisited', '#E77BD5');
        document.querySelectorAll("pre").forEach(element => {
            element.classList.remove("light");
        })
        //root.style.setProperty('--aliasName', 'gold');
    }
    darkToggle.addEventListener('click', () => {
        toggleDarkTheme()
    })

    function toggleDarkTheme() {
        if (darkToggle.classList.contains("light")) {
            setDark()
            darkToggle.classList.remove("light")
        } else {
            darkToggle.classList.add("light")
            setLight()
        }
    }

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

    function createLinks() {
        const keywords = document.querySelectorAll(".hljs-keyword");
        keywords.forEach(keyword => {
            if (keyword.textContent == "alias") {
                let link = document.createElement("a");
                link.href = "/script/alias.html";
                link.innerHTML = keyword.outerHTML;
                keyword.parentNode.insertBefore(link, keyword)
                keyword.remove()
            } else if (keyword.textContent == "for") {
                let link = document.createElement("a");
                link.href = "/script/for.html";
                link.innerHTML = keyword.outerHTML;
                keyword.parentNode.insertBefore(link, keyword)
                keyword.remove()
            } else if (keyword.textContent == "if") {
                let link = document.createElement("a");
                link.href = "/script/if.html";
                link.innerHTML = keyword.outerHTML;
                keyword.parentNode.insertBefore(link, keyword)
                keyword.remove()
            } else if (keyword.textContent == "enum") {
                let link = document.createElement("a");
                link.href = "/script/enum.html";
                link.innerHTML = keyword.outerHTML;
                keyword.parentNode.insertBefore(link, keyword)
                keyword.remove()
            } else if (keyword.textContent == "function") {
                let link = document.createElement("a");
                link.href = "/script/function.html";
                link.innerHTML = keyword.outerHTML;
                keyword.parentNode.insertBefore(link, keyword)
                keyword.remove()
            }
        })
    }


    window.onload = function() {
        sidebar.classList.add("afterload");
        html.classList.add("afterload");
        createLinks()
    }
})();
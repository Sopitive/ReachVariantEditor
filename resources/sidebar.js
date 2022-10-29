(function() {
    const sidebar = document.querySelector("#sidebar");
    const main = document.querySelector("main");
    const collapse = document.createElement("button");
    const lastItem = document.querySelector("ul:last-child");
    const darkToggle = document.createElement("button");
    const body = document.querySelector("body");
    darkToggle.classList.add("dark-toggle");
    darkToggle.textContent = "Light Mode"
    lastItem.appendChild(darkToggle);
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
            sidebar.classList.remove("hide");
            localStorage.setItem("collapse", "Expanded");
        } else {
            localStorage.setItem("collapse", "Collapsed");
            sidebar.classList.add("hide");
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
    } else {
        sidebar.classList.add("hide");
        if (localStorage.getItem("darkMode" == "Dark")) {
            body.classList.add("hide-color")
        }
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
        root.style.setProperty('--mainColor', '#EEE');
        root.style.setProperty('--secondMain', '#fff');
        root.style.setProperty('--textColor', '#000');
        root.style.setProperty('--sidebar-back', '#EEE');
        root.style.setProperty('--sidebar-foreground', '#fff');
        root.style.setProperty('--scrollbar', '#EEE');
        root.style.setProperty('--scrollbarThumb', '#C1C1C1');
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
    window.onload = function() {
        sidebar.classList.add("afterload");
    }
})();
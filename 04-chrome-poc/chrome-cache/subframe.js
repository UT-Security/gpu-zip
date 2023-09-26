// receive parent frame message and resize
window.addEventListener("message", e => {
  scroll = document.getElementById("scroll");
  scroll && (scroll.style.transform = `scale(${e.data.size})`);
  let innerframe = document.getElementById('frameinner');
  innerframe && (innerframe.style.width = e.data.size + 'px') && (innerframe.style.height = e.data.size + 'px');
}, false);


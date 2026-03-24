function handleLogin(event) {
    event.preventDefault();
    
    const username = document.getElementById('username').value;
    const password = document.getElementById('password').value;
    const errorMsg = document.getElementById('errorMsg');
    
    fetch('/api/login', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
        },
        body: JSON.stringify({
            username: username,
            password: password
        })
    })
    .then(response => response.json())
    .then(data => {
        console.log('Login response:', data);
        if (data.success) {
            // Redirect to root (which will serve index.html after auth check)
            window.location.href = '/';
        } else {
            errorMsg.style.display = 'block';
            setTimeout(() => {
                errorMsg.style.display = 'none';
            }, 3000);
        }
    })
    .catch(error => {
        console.error('Login error:', error);
        errorMsg.style.display = 'block';
        errorMsg.textContent = 'Connection error';
        setTimeout(() => {
            errorMsg.style.display = 'none';
        }, 3000);
    });
    
    return false;
}
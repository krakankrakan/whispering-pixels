package com.example.testapp

import android.content.Intent
import android.graphics.Color
import android.os.Bundle
import android.view.View
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.tooling.preview.Preview
import androidx.core.view.ViewCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.WindowInsetsControllerCompat
import com.example.testapp.ui.theme.TestAppTheme
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

class MainActivity : ComponentActivity() {
    external fun runComputeShader()

    private var job: Job? = null
    private var job2: Job? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContent {
            TestAppTheme {
                // A surface container using the 'background' color from the theme
                Surface(modifier = Modifier.fillMaxSize(), color = MaterialTheme.colorScheme.background) {
                    Greeting("Android")
                }
            }
        }

        System.loadLibrary("p_a_opencl")
        //runComputeShader()

        job = CoroutineScope(Dispatchers.Default).launch {
            while (isActive) {
                runComputeShader()
                delay(1)
            }
        }

        /*job2 = CoroutineScope(Dispatchers.Default).launch {
            while (isActive) {
                withContext(Dispatchers.Main) {
                    showToast("hello bois")

                    delay(1000)
                    val windowInsetsController =
                        ViewCompat.getWindowInsetsController(window.decorView)
                    windowInsetsController?.systemBarsBehavior =
                        WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
                    windowInsetsController?.hide(WindowInsetsCompat.Type.systemBars())
                    delay(1000)
                    windowInsetsController?.show(WindowInsetsCompat.Type.systemBars())
                }
            }
        }*/
    }

    fun showToast(message: String) {
        // Replace 'this' with the appropriate context
        Toast.makeText(applicationContext, message, Toast.LENGTH_SHORT).show()
    }

    override fun onDestroy() {
        super.onDestroy()
        job?.cancel()
    }
}

@Composable
fun Greeting(name: String, modifier: Modifier = Modifier) {
    Text(
            text = "Hello $name!",
            modifier = modifier
    )
}

@Preview(showBackground = true)
@Composable
fun GreetingPreview() {
    TestAppTheme {
        Greeting("Android")
    }
}